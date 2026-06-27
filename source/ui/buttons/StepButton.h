#pragma once

#include "../StateListenerComponent.h"
#include "../MusicConstants.h"
#include "../MidiLearnOverlayPainter.h"
#include "../UIConstants.h"
#include "../../backend/MidiLearnRegistry/MidiLearnRegistry.h"
#include "../../data/Params.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>

namespace StepButtonConstants {
    constexpr float cornerRadius = 4.4f;
    constexpr float borderWidthSelected = 3.0f;  // Border width for selected step
    constexpr float fontSize = 13.0f;
    constexpr float activeCircleRadius = 20.0f;
    constexpr float colourFadeSpeed = 8.0f;  // Lerp speed (higher = faster fade)

    // Colors
    const juce::Colour restStepColor = juce::Colour(0xff505050);      // Lighter grey for rest steps
    const juce::Colour emptyStepColor = juce::Colour(0xff2a2a2a);     // Darker grey for empty steps
    const juce::Colour textColorOnNote = juce::Colours::black;        // Black text on colored notes
    const juce::Colour textColorOnCurrent = juce::Colours::black;     // Black text on white current step
}

/**
 * StepButton - Individual step button for MIDI-mappable step selection
 *
 * Represents one of 8 visible steps in the sequencer window.
 * Each button has a fixed local index (0-7) that maps to STEP_0 through STEP_7
 * for MIDI learn purposes.
 *
 * The absolute step index is calculated as: windowStart + localIndex
 *
 * Visual States:
 * - Current (white fill + black circle with note name) - This step is currently playing
 * - Selected (white border) - This step is selected for editing
 * - Normal (note color fill) - Regular step with note color
 * - Rest (grey fill) - Step contains a rest (-1)
 *
 * Observer Pattern:
 * - Listens to step note parameters for this step
 * - Listens to sequence metadata (currentIndex, selectedStep, windowStart)
 * - Automatically repaints when subscribed parameters change
 */
class StepButton : public StateListenerComponent {
public:
    StepButton(PluginProcessor& processor, int localIndex)
        : StateListenerComponent(processor)
        , localIndex_(localIndex)
    {
        // Subscribe to all step notes (we might display any step depending on window)
        listenToStepNotes(0, 63);
        listenToStepNotesLength(0, 63);

        // Subscribe to sequence metadata
        listenToSequenceMetadata();

        // Subscribe to note colour changes
        for (int i = 0; i < 12; ++i)
            listenToParameter(Params::Keys::ID::noteColour(i));

        // Enable mouse clicks
        setInterceptsMouseClicks(true, false);
    }
    
    ~StepButton() override = default;
    
    /**
     * Set the MIDI control ID for this button (STEP_0 through STEP_7)
     */
    void setMidiControlId(MidiControlId controlId) {
        midiControlId_ = controlId;
    }

    /**
     * Set the MIDI learn registry
     */
    void setRegistry(MidiLearnRegistry* reg) {
        registry_ = reg;
    }

    /**
     * Get the local index (0-7)
     */
    int getLocalIndex() const {
        return localIndex_;
    }
    
    /**
     * Get the absolute step index based on current window position
     */
    int getAbsoluteStepIndex() const {
        int windowStart = getState().getViewWindowStart();
        return windowStart + localIndex_;
    }
    
    /**
     * Callback when this step button is clicked
     * Receives the absolute step index (0-63)
     */
    std::function<void(int absoluteStepIndex)> onStepClicked;

    /**
     * Callback when the step button is released (mouseUp).
     * Used to send a MIDI note-off so a tapped step doesn't leave a stuck note.
     */
    std::function<void(int absoluteStepIndex)> onStepReleased;
    
    void paint(juce::Graphics& g) override {
        using namespace StepButtonConstants;
        auto& state = getState();

        int absoluteStepIndex = getAbsoluteStepIndex();

        // Check if this step is within the active sequence length
        bool isWithinSequence = absoluteStepIndex < state.getLength();

        // Check if this is the current step (playback position)
        bool isCurrent = isWithinSequence && (absoluteStepIndex == state.getCurrentIndex());

        // Check if this is the selected step for editing
        bool isSelected = (absoluteStepIndex == state.getSelectedStepForEditing());

        auto bounds = getLocalBounds().toFloat();

        // Determine the target fill color
        juce::Colour targetColor;
        int midiNote = -1;
        bool isRest = true;

        if (isWithinSequence) {
            midiNote = state.getCurrentNoteForStep(absoluteStepIndex);
            isRest = (midiNote < 0);

            if (isCurrent) {
                targetColor = UIConstants::activeStepColour;
            } else if (isRest) {
                targetColor = StepButtonConstants::restStepColor;
            } else {
                int chromaticNote = midiNote % 12;
                targetColor = getState().getNoteColour(chromaticNote);
            }
        } else {
            // Out-of-bounds step (beyond sequence length): no solid fill —
            // rendered as a dashed placeholder outline below.
            targetColor = juce::Colours::transparentBlack;
        }

        // Animate: lerp displayColour_ toward targetColor
        if (!displayColourInitialised_) {
            displayColour_ = targetColor;
            displayColourInitialised_ = true;
        } else if (displayColour_ != targetColor) {
            float t = juce::jmin(1.0f, colourFadeSpeed * frameDeltaSec_);
            displayColour_ = displayColour_.interpolatedWith(targetColor, t);
            // Keep repainting until we converge
            if (displayColour_ != targetColor)
                needsAnimationRepaint_ = true;
        }

        // Fill background with animated color
        g.setColour(displayColour_);
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Out-of-bounds steps get a dashed placeholder outline instead of a fill.
        if (!isWithinSequence) {
            UIConstants::drawPlaceholderOutline(g, bounds, cornerRadius);
        }

        if (isWithinSequence) {
            // Animate border width toward target
            float borderTarget = isSelected ? borderWidthSelected : 0.0f;
            if (std::abs(borderDisplayWidth_ - borderTarget) > 0.01f) {
                float duration = (borderTarget > borderDisplayWidth_)
                    ? UIConstants::borderFadeInDuration
                    : UIConstants::borderFadeOutDuration;
                float step = frameDeltaSec_ / duration;
                borderDisplayWidth_ = (borderTarget > borderDisplayWidth_)
                    ? juce::jmin(borderDisplayWidth_ + step * borderWidthSelected, borderTarget)
                    : juce::jmax(borderDisplayWidth_ - step * borderWidthSelected, borderTarget);
                needsAnimationRepaint_ = true;
            } else {
                borderDisplayWidth_ = borderTarget;
            }

            // Draw animated border
            if (borderDisplayWidth_ > 0.01f) {
                g.setColour(UIConstants::selectedStepOutlineColour);
                float inset = borderDisplayWidth_ * 0.5f;
                juce::Rectangle<float> borderBounds = bounds.reduced(inset);
                float borderRadiusInner = juce::jmax(0.0f, cornerRadius - inset);
                g.drawRoundedRectangle(borderBounds, borderRadiusInner, borderDisplayWidth_);
            }

            // Draw note text (only for non-rest steps)
            if (!isRest) {
                drawNoteText(g, bounds, midiNote, isCurrent);
            }

            // Draw active circle indicator if this is the playing step (skip for rests)
            if (isCurrent && !isRest) {
                drawActiveCircle(g, bounds, midiNote);
            }
        }

        // Draw MIDI learn overlay on top of everything
        paintMidiLearnOverlay(g);
    }

    void mouseDown(const juce::MouseEvent& event) override {
        if (event.mods.isLeftButtonDown()) {
            // MIDI-learn capture happens immediately on press.
            if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
                registry_->setLearningControl(midiControlId_);
                repaint();
                return;
            }
            // Audition fires on release (mouseUp). Deferring it lets the
            // parent SequencerStepsComponent's drag-to-set-length gesture
            // suppress the preview cleanly: if the release lands on a
            // different button, JUCE delivers mouseUp here with a position
            // outside our local bounds, and we skip the audition.
        }
    }

    void mouseUp(const juce::MouseEvent& event) override {
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT)
            return;

        // Release outside our own bounds means the user dragged off this
        // button — treat it as a drag, not a click, and skip the audition.
        if (!getLocalBounds().contains(event.getPosition()))
            return;

        int absoluteStepIndex = getAbsoluteStepIndex();
        if (absoluteStepIndex < 64 && onStepClicked)
            onStepClicked(absoluteStepIndex);
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override {
        if (event.mods.isLeftButtonDown()) {
            // Double-click forces the current step to jump to this step
            int absoluteStepIndex = getAbsoluteStepIndex();
            if (absoluteStepIndex < 64) {
                auto& state = getState();
                // Only allow jumping to steps within the sequence length
                if (absoluteStepIndex < state.getLength()) {
                    state.setCurrentIndex(absoluteStepIndex);
                }
            }
        }
    }

    void parentHierarchyChanged() override {
        // Set up VBlank-driven animation when added to a visible parent
        if (getTopLevelComponent() != nullptr && vBlankAttachment_ == nullptr) {
            vBlankAttachment_ = std::make_unique<juce::VBlankAttachment>(
                this, [this](double timestampSec) {
                    double delta = (lastVBlankTimestamp_ > 0.0)
                        ? (timestampSec - lastVBlankTimestamp_)
                        : (1.0 / 60.0);
                    lastVBlankTimestamp_ = timestampSec;
                    frameDeltaSec_ = static_cast<float>(juce::jlimit(0.001, 0.1, delta));

                    if (needsAnimationRepaint_) {
                        needsAnimationRepaint_ = false;
                        repaint();
                    }
                });
        }
    }

    void onStateChanged() override {
        // Repaint when any subscribed parameter changes
        repaint();
    }

private:
    void drawNoteText(juce::Graphics& g, const juce::Rectangle<float>& bounds, int midiNote, bool isCurrent) {
        using namespace StepButtonConstants;

        // Convert MIDI note to note name (e.g., "C4", "Gb3")
        juce::String noteName = MusicConstants::midiNoteToNoteName(midiNote);

        // Text color: always black (works on both white current step and colored note backgrounds)
        g.setColour(isCurrent ? textColorOnCurrent : textColorOnNote);

        // Load Inter font and make it bold
        auto interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
        juce::Font font(juce::FontOptions(interFont).withHeight(fontSize));
        font.setBold(true);
        g.setFont(font);

        // Center the text
        g.drawText(noteName, bounds, juce::Justification::centred, false);
    }

    void drawActiveCircle(juce::Graphics& g, const juce::Rectangle<float>& bounds, int midiNote) {
        using namespace StepButtonConstants;

        // Draw black circle with white text showing note name (e.g., "A7", "Bb2")
        float circleDiameter = activeCircleRadius * 2.0f;
        float circleX = bounds.getCentreX() - activeCircleRadius;
        float circleY = bounds.getCentreY() - activeCircleRadius;

        juce::Rectangle<float> circleBounds(circleX, circleY, circleDiameter, circleDiameter);

        // Draw black circle
        g.setColour(juce::Colours::black);
        g.fillEllipse(circleBounds);

        // Draw white text (note name like "A7" or "Bb2")
        g.setColour(juce::Colours::white);

        // Load Inter font and make it bold
        auto interFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
        juce::Font font(juce::FontOptions(interFont).withHeight(fontSize));
        font.setBold(true);
        g.setFont(font);

        // Convert MIDI note to note name (e.g., "C4", "Gb3")
        juce::String noteName = MusicConstants::midiNoteToNoteName(midiNote);
        g.drawText(noteName, circleBounds, juce::Justification::centred, false);
    }

    void paintMidiLearnOverlay(juce::Graphics& g) {
        MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_,
                                      StepButtonConstants::cornerRadius);
    }

    int localIndex_;  // 0-7 (fixed, never changes)
    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;

    // Border fade animation state
    float borderDisplayWidth_ = 0.0f;

    // Colour fade animation state
    juce::Colour displayColour_ { StepButtonConstants::emptyStepColor };
    bool displayColourInitialised_ = false;
    bool needsAnimationRepaint_ = false;
    float frameDeltaSec_ = 1.0f / 60.0f;
    double lastVBlankTimestamp_ = 0.0;
    std::unique_ptr<juce::VBlankAttachment> vBlankAttachment_;
};


