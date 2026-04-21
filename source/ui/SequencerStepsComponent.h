#pragma once

#include "../PluginProcessor.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "SequenceGrid/SequenceGrid.h"
#include "StateListenerComponent.h"
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

            addAndMakeVisible(*button);
            stepButtons_[i] = std::move(button);
        }

        setSize (componentWidth, componentHeight);
    }

    ~SequencerStepsComponent() override = default;

    void paint (juce::Graphics& /*g*/) override {
        // Buttons handle their own painting
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

    /**
     * Callback when a step is clicked
     * Receives the absolute step index (0-63)
     */
    std::function<void(int stepIndex)> onStepClicked;

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

    std::array<std::unique_ptr<StepButton>, SequencerStepsConstants::numVisibleSteps> stepButtons_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerStepsComponent)
};

