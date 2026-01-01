#pragma once

#include "../PluginProcessor.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "SequenceGrid.h"

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
    // Border (only for active step)
    constexpr float borderWidthActive = 3.0f;
    constexpr float cornerRadius = 4.4f;
    // Text
    constexpr float fontSize = 13.0f;
    // Active circle
    constexpr float activeCircleRadius = 20.0f;
}

/**
 * SequencerStepsComponent - Displays 8 steps of the 64-step sequence
 *
 * This is a "window" into the full sequence, showing 8 consecutive steps.
 * Each step displays:
 * - Background color from the note's scale color
 * - Note number (MIDI note 0-127)
 * - Active step indicator: black circle (40px diameter) with white text
 *
 * The window can be moved through the sequence to view different sections.
 */
class SequencerStepsComponent : public juce::Component {
public:
    explicit SequencerStepsComponent(PluginProcessor& processor) : processorRef(processor) {
        using namespace SequencerStepsConstants;

        // Load Inter font from binary data
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        setSize (componentWidth, componentHeight);
    }

    ~SequencerStepsComponent() override = default;

    void paint (juce::Graphics& g) override {
        using namespace SequencerStepsConstants;

        // Draw each visible step
        for (int i = 0; i < numVisibleSteps; i++) {
            int stepIndex = windowStartIndex + i;
            if (stepIndex >= 64) break; // Don't draw beyond sequence length

            drawStep (g, i, stepIndex, stepSize, gap);
        }
    }

    void resized() override {
        // Nothing to do - we handle all rendering in paint()
    }

    /**
     * Set which step is currently active (0-63, or -1 for none)
     */
    void setActiveStep (int stepIndex) {
        if (activeStepIndex != stepIndex) {
            activeStepIndex = stepIndex;
            repaint();
        }
    }

    /**
     * Get the active step index (-1 if none)
     */
    int getActiveStep() const {
        return activeStepIndex;
    }

    /**
     * Set which step is currently selected for editing (0-63, or -1 for none)
     */
    void setSelectedStep (int stepIndex) {
        if (selectedStepIndex != stepIndex) {
            selectedStepIndex = stepIndex;
            repaint();
        }
    }

    /**
     * Get the selected step index (-1 if none)
     */
    int getSelectedStep() const {
        return selectedStepIndex;
    }

    /**
     * Set the window start position (0-56, to show steps 0-7 through 56-63)
     */
    void setWindowStart (int startIndex) {
        if (startIndex >= 0 && startIndex <= 56 && windowStartIndex != startIndex) {
            windowStartIndex = startIndex;
            repaint();
        }
    }

    /**
     * Get the window start index
     */
    int getWindowStart() const {
        return windowStartIndex;
    }

    /**
     * Callback when a step is clicked
     */
    std::function<void(int stepIndex)> onStepClicked;

private:
    /**
     * Draw a single step
     */
    void drawStep (juce::Graphics& g, int visibleIndex, int stepIndex, float stepSizeParam, float gapParam) {
        using namespace SequencerStepsConstants;

        // Get state from APVTS
        auto& state = processorRef.getState();

        // Calculate step bounds
        float x = visibleIndex * (stepSizeParam + gapParam);
        float y = 0.0f;

        juce::Rectangle<float> stepBounds (x, y, stepSizeParam, stepSizeParam);

        // Check if this step is within the active sequence length
        bool isWithinSequence = stepIndex < state.getLength();

        // Check if this is the active step (playback position, only if within sequence)
        bool isActive = isWithinSequence && (stepIndex == activeStepIndex);

        // Check if this is the selected step for editing
        bool isSelected = (stepIndex == selectedStepIndex);

        if (isWithinSequence) {
            // Check if this step's current note is a rest (-1)
            bool isRest = state.stepHasRest(stepIndex);

            // Get the current note for this step
            int midiNote = state.getCurrentNoteForStep(stepIndex);

            // Determine the fill color
            juce::Colour fillColor;
            if (isSelected) {
                // Selected step for editing - white fill
                fillColor = juce::Colours::white;
            } else if (isRest) {
                // Rest step - grey fill
                fillColor = juce::Colour (SequenceGridConstants::defaultGreyHex);
            } else {
                // Normal step - note color fill
                int chromaticNote = midiNote % 12;
                fillColor = getNoteColor (chromaticNote);
            }

            // Fill step background
            g.setColour (fillColor);
            g.fillRoundedRectangle (stepBounds, cornerRadius);

            // Draw white border on active step (playback position)
            if (isActive) {
                g.setColour (juce::Colours::white);
                float inset = borderWidthActive * 0.5f;
                juce::Rectangle<float> borderBounds = stepBounds.reduced (inset);
                float borderRadiusInner = juce::jmax (0.0f, cornerRadius - inset);
                g.drawRoundedRectangle (borderBounds, borderRadiusInner, borderWidthActive);
            }

            // Draw note text (only for non-rest steps)
            if (!isRest) {
                // Convert MIDI note to note name (e.g., "C4", "Gb3")
                juce::String noteText = midiNoteToNoteName (midiNote);

                // Use Inter font
                juce::Font font (interFont);
                font.setHeight (fontSize);
                font.setBold (true);

                // Determine text color based on selection and active state
                if (isSelected) {
                    // Selected step has white fill, so use black text
                    // But if also active, draw the black circle with white text
                    if (isActive) {
                        // Draw black circle behind the text
                        auto circleCentre = stepBounds.getCentre();
                        g.setColour (juce::Colours::black);
                        g.fillEllipse (circleCentre.x - activeCircleRadius, circleCentre.y - activeCircleRadius,
                                      activeCircleRadius * 2.0f, activeCircleRadius * 2.0f);
                        g.setColour (juce::Colours::white);
                    } else {
                        g.setColour (juce::Colours::black);
                    }
                } else if (isActive) {
                    // Draw black circle behind the text
                    auto circleCentre = stepBounds.getCentre();
                    g.setColour (juce::Colours::black);
                    g.fillEllipse (circleCentre.x - activeCircleRadius, circleCentre.y - activeCircleRadius,
                                  activeCircleRadius * 2.0f, activeCircleRadius * 2.0f);
                    g.setColour (juce::Colours::white);
                } else {
                    // Normal step - black text
                    g.setColour (juce::Colours::black);
                }

                g.setFont (font);
                g.drawText (noteText, stepBounds.toNearestInt(), juce::Justification::centred);
            }
        } else {
            // Step is outside the active sequence - draw even darker grey with no note text
            g.setColour (juce::Colour (SequenceGridConstants::outOfBoundsGreyHex));
            g.fillRoundedRectangle (stepBounds, cornerRadius);

            // No border for inactive steps outside sequence
        }
    }

    /**
     * Convert MIDI note number to note name (e.g., 60 -> "C4", 67 -> "G4")
     */
    juce::String midiNoteToNoteName (int midiNote) const {
        const char* noteNames[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};

        int chromaticNote = midiNote % 12;
        int octave = (midiNote / 12) - 1; // MIDI note 60 = C4

        return juce::String (noteNames[chromaticNote]) + juce::String (octave);
    }

    /**
     * Get the color for a chromatic note (0-11)
     * Same color mapping as PianoRollComponent
     */
    juce::Colour getNoteColor (int chromaticNote) const {
        const juce::Colour noteColors[12] = {
            juce::Colour (0xFFFF6B6B), // C - Red
            juce::Colour (0xFFFF9F6B), // Db - Orange
            juce::Colour (0xFFFFD66B), // D - Yellow
            juce::Colour (0xFFFFE66B), // Eb - Light yellow
            juce::Colour (0xFFE6FF6B), // E - Yellow-green
            juce::Colour (0xFF6BFFB4), // F - Light green
            juce::Colour (0xFF6BFFD6), // Gb - Cyan-green
            juce::Colour (0xFF6BD6FF), // G - Cyan
            juce::Colour (0xFF6BB4FF), // Ab - Light blue
            juce::Colour (0xFF9F6BFF), // A - Purple
            juce::Colour (0xFFD66BFF), // Bb - Magenta
            juce::Colour (0xFFFF6BD6)  // B - Pink
        };

        return noteColors[chromaticNote % 12];
    }

    void mouseDown (const juce::MouseEvent& event) override {
        using namespace SequencerStepsConstants;
        const float stepPlusGap = stepSize + gap;

        int visibleIndex = static_cast<int>(event.position.x / stepPlusGap);
        if (visibleIndex >= 0 && visibleIndex < numVisibleSteps) {
            int stepIndex = windowStartIndex + visibleIndex;
            if (stepIndex < 64 && onStepClicked) {
                onStepClicked (stepIndex);
            }
        }
    }

    juce::Typeface::Ptr interFont;
    PluginProcessor& processorRef; // Reference to the processor for state access
    int activeStepIndex = -1; // -1 means no active step (playback position), 0-63 for sequence steps
    int selectedStepIndex = 0; // Which step is selected for editing (0-63)
    int windowStartIndex = 0; // Which step is at the left edge of the window (0-56)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerStepsComponent)
};

