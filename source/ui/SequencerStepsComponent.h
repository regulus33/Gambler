#pragma once

#include "../PluginProcessor.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "SequenceGrid/SequenceGrid.h"
#include "SequenceGrid/SequenceGridConstants.h"
#include "StateListenerComponent.h"
#include "UIConstants.h"
#include "buttons/StepButton.h"
#include <array>

// Forward declaration
class PluginProcessor;

namespace SequencerStepsConstants {
    // Component size
    constexpr int componentWidth = 862;
    constexpr int componentHeight = 92;
    // Layout
    constexpr int numVisibleSteps = 8;
    constexpr float stepSize = 92.0f;
    constexpr float gap = 18.0f;
}

/**
 * SequencerStepsComponent - Displays 8 steps of the 64-step sequence
 *
 * This is a "window" into the full sequence, showing 8 consecutive steps.
 * Each step displays:
 * - Background color from the note's scale color
 * - Note name + octave (e.g., "A7", "Bb2")
 * - Current step indicator: white fill + black circle (40px diameter) with note name
 * - Selected step indicator: white border
 *
 * The window can be moved through the sequence to view different sections.
 *
 * Observer Pattern:
 * - Listens to all step note parameters (64 steps × 8 substeps)
 * - Listens to sequence metadata (length, currentIndex, selectedStep)
 * - Automatically repaints when any subscribed parameter changes
 */
class SequencerStepsComponent : public StateListenerComponent {
public:
    explicit SequencerStepsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor) {
        using namespace SequencerStepsConstants;

        // Create 8 step buttons with local indices 0-7
        for (int i = 0; i < numVisibleSteps; i++) {
            auto button = std::make_unique<StepButton>(processor, i);

            // Set MIDI control ID (STEP_0 through STEP_7)
            button->setMidiControlId(static_cast<MidiControlId>(
                static_cast<int>(MidiControlId::STEP_0) + i));

            // Wire up click callback
            button->onStepClicked = [this](int absoluteStepIndex) {
                if (onStepClicked) {
                    onStepClicked(absoluteStepIndex);
                }
            };

            // Wire up release callback (mouseUp) — used to send MIDI note-off
            // so an auditioned step doesn't leave a stuck note.
            button->onStepReleased = [this](int absoluteStepIndex) {
                if (onStepReleased) {
                    onStepReleased(absoluteStepIndex);
                }
            };

            addAndMakeVisible(*button);
            // Forward the button's mouse events to this parent so we can
            // detect a click-and-drag across multiple buttons (the same
            // gesture SequenceGrid uses to set sequence length). The button
            // still handles its own click/release for note auditioning;
            // we just observe so we can paint a drag-preview overlay and
            // commit the new length on release.
            button->addMouseListener(this, false);
            stepButtons_[i] = std::move(button);
        }

        setSize (componentWidth, componentHeight);
    }

    ~SequencerStepsComponent() override = default;

    void paint (juce::Graphics& /*g*/) override {
        // Buttons handle their own painting
    }

    void paintOverChildren (juce::Graphics& g) override {
        if (!isDraggingLength_) return;

        for (int i = 0; i < SequencerStepsConstants::numVisibleSteps; ++i) {
            auto bounds = stepButtons_[i]->getBounds().toFloat();

            if (i <= dragTargetLocalIndex_) {
                // Will remain in-bounds: orange preview fill.
                g.setColour(juce::Colour(SequenceGridConstants::dragPreviewOrangeHex));
                g.fillRoundedRectangle(bounds, StepButtonConstants::cornerRadius);

                if (i == dragTargetLocalIndex_) {
                    g.setColour(UIConstants::stepIndicatorWhite.withAlpha(0.5f));
                    g.drawRoundedRectangle(bounds.reduced(1.0f),
                                           StepButtonConstants::cornerRadius, 1.5f);
                }
            } else {
                // Will fall out of bounds: preview as a dashed placeholder.
                // The step buttons underneath are opaque, so first paint over
                // them with the app background (what a real out-of-bounds step's
                // transparent fill reveals), then draw the dashed outline.
                g.setColour(UIConstants::appBackgroundColour);
                g.fillRoundedRectangle(bounds, StepButtonConstants::cornerRadius);
                UIConstants::drawPlaceholderOutline(g, bounds, StepButtonConstants::cornerRadius);
            }
        }
    }

    void resized() override {
        using namespace SequencerStepsConstants;

        // Layout the 8 step buttons horizontally with gaps
        for (int i = 0; i < numVisibleSteps; i++) {
            float x = i * (stepSize + gap);
            stepButtons_[i]->setBounds(
                static_cast<int>(x),
                0,
                static_cast<int>(stepSize),
                static_cast<int>(stepSize)
            );
        }
    }

    // ===== Drag-to-set-length (parent-level mouse listener on each button) ====
    //
    // Mirrors SequenceGrid's gesture: press a step, drag across to another
    // step, release — sequence length becomes (lastDragStep + 1). Only enabled
    // when the visible window is the first row (steps 0–7), so the local index
    // and the absolute step index are identical.
    void mouseDown (const juce::MouseEvent& event) override {
        if (!isFirstRow()) return;

        auto p = event.getEventRelativeTo(this).getPosition();
        mouseDownLocalIndex_ = localIndexAtPosition(p);
        dragTargetLocalIndex_ = mouseDownLocalIndex_;
        isDraggingLength_ = false;
    }

    void mouseDrag (const juce::MouseEvent& event) override {
        if (!isFirstRow() || mouseDownLocalIndex_ < 0) return;

        auto p = event.getEventRelativeTo(this).getPosition();
        int hovered = localIndexAtPosition(p);
        if (hovered < 0) return;

        if (!isDraggingLength_ && hovered != mouseDownLocalIndex_) {
            isDraggingLength_ = true;
        }
        if (isDraggingLength_ && hovered != dragTargetLocalIndex_) {
            dragTargetLocalIndex_ = hovered;
            repaint();
        }
    }

    void mouseUp (const juce::MouseEvent& /*event*/) override {
        if (isDraggingLength_) {
            if (onLengthDragged && dragTargetLocalIndex_ >= 0)
                onLengthDragged(dragTargetLocalIndex_ + 1);
            isDraggingLength_ = false;
            repaint();
        }
        mouseDownLocalIndex_ = -1;
        dragTargetLocalIndex_ = -1;
    }

    /**
     * Callback when the user drags across step buttons to set sequence length.
     * Receives the new length (1–8). Only fires when the first row is visible.
     */
    std::function<void(int newLength)> onLengthDragged;

    /**
     * Callback when a step is clicked
     * Receives the absolute step index (0-63)
     */
    std::function<void(int stepIndex)> onStepClicked;

    /**
     * Callback when a step button is released (mouseUp).
     * Receives the absolute step index (0-63). Used to send MIDI note-off
     * for the audition press.
     */
    std::function<void(int stepIndex)> onStepReleased;

    /**
     * Get access to individual step buttons (for MIDI learn registration, etc.)
     */
    StepButton* getStepButton(int localIndex) {
        using namespace SequencerStepsConstants;
        if (localIndex >= 0 && localIndex < numVisibleSteps) {
            return stepButtons_[localIndex].get();
        }
        return nullptr;
    }

    /**
     * Set the MIDI learn registry for all step buttons
     */
    void setRegistry(MidiLearnRegistry* registry) {
        using namespace SequencerStepsConstants;
        for (int i = 0; i < numVisibleSteps; i++) {
            stepButtons_[i]->setRegistry(registry);
        }
    }

private:
    // ========================================
    // StateListenerComponent implementation
    // ========================================

    /**
     * Called on message thread when any subscribed parameter changes.
     * Buttons handle their own state updates, so we don't need to do anything here.
     */
    void onStateChanged() override {
        // Buttons automatically repaint themselves via their own StateListenerComponent
    }

    bool isFirstRow() const {
        return getState().getViewWindowStart() == 0;
    }

    int localIndexAtPosition (juce::Point<int> p) const {
        for (int i = 0; i < SequencerStepsConstants::numVisibleSteps; ++i) {
            if (stepButtons_[i]->getBounds().contains(p)) return i;
        }
        return -1;
    }

    int  mouseDownLocalIndex_ = -1;
    int  dragTargetLocalIndex_ = -1;
    bool isDraggingLength_ = false;

    std::array<std::unique_ptr<StepButton>, SequencerStepsConstants::numVisibleSteps> stepButtons_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerStepsComponent)
};

