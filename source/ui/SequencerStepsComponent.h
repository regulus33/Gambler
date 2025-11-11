#pragma once

#include "../backend/Sequence.h"
#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SequencerStepsComponent - Displays 8 steps of the 64-step sequence
 *
 * This is a "window" into the full sequence, showing 8 consecutive steps.
 * Each step displays:
 * - Background color from the note's scale color
 * - Note number (MIDI note 0-127)
 * - Active step indicator: black circle (26px diameter) with white text
 *
 * The window can be moved through the sequence to view different sections.
 */
class SequencerStepsComponent : public juce::Component {
public:
    SequencerStepsComponent(const Sequence& seq) : sequence(seq) {
        // Load Inter font from binary data
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Set fixed size based on design:
        // 8 steps × 84px + 7 gaps × 16px = 672 + 112 = 784px wide
        // Height: 84px
        setSize (784, 84);
    }

    ~SequencerStepsComponent() override = default;

    void paint (juce::Graphics& g) override {
        // No background - transparent

        // Step dimensions from design
        const int numVisibleSteps = 8;
        const float stepSize = 84.0f; // 84x84 pixels
        const float gap = 16.0f; // 16px gap between steps

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
    void drawStep (juce::Graphics& g, int visibleIndex, int stepIndex, float stepSize, float gap) {
        // Calculate step bounds
        // Each step is 84x84, with 16px gap between them
        float x = visibleIndex * (stepSize + gap);
        float y = 0.0f;
        float w = stepSize;
        float h = stepSize;

        juce::Rectangle<float> stepBounds (x, y, w, h);

        // Get the note for this step
        const auto& step = sequence.getStep(stepIndex);
        const auto& stepNotes = step.getStepNotes();
        const auto& currentNote = stepNotes.getCurrentNote();

        // Get the current note to display
        int midiNote = currentNote.getNoteNumber();

        // Get the note color from the chromatic scale (0-11)
        int chromaticNote = midiNote % 12;
        juce::Colour noteColor = getNoteColor (chromaticNote);

        // Check if this is the active step
        bool isActive = (stepIndex == activeStepIndex);

        // Determine border width (2px for inactive, 6px for active)
        float borderWidth = isActive ? 6.0f : 2.0f;
        float cornerRadius = 4.0f;

        // Fill step background with note color
        g.setColour (noteColor);
        g.fillRoundedRectangle (stepBounds, cornerRadius);

        // Draw white border INSIDE the bounds
        // Inset by half the border width so the border draws entirely inside
        // Also reduce the corner radius proportionally to match the inset
        g.setColour (juce::Colours::white);
        float inset = borderWidth * 0.5f;
        juce::Rectangle<float> borderBounds = stepBounds.reduced (inset);
        float borderRadius = juce::jmax (0.0f, cornerRadius - inset);
        g.drawRoundedRectangle (borderBounds, borderRadius, borderWidth);

        // Convert MIDI note to note name (e.g., "C4", "Gb3")
        juce::String noteText = midiNoteToNoteName (midiNote);

        // Use Inter font at 12px bold
        juce::Font font (interFont);
        font.setHeight (12.0f);
        font.setBold (true);

        if (isActive) {
            // Draw black circle (36px diameter) behind the text
            auto circleCentre = stepBounds.getCentre();
            const float circleRadius = 18.0f; // 36px diameter = 18px radius
            g.setColour (juce::Colours::black);
            g.fillEllipse (circleCentre.x - circleRadius, circleCentre.y - circleRadius,
                          circleRadius * 2.0f, circleRadius * 2.0f);

            // Draw note text in white, bold Inter font
            g.setColour (juce::Colours::white);
            g.setFont (font);
        } else {
            // Draw note text in black, bold Inter font
            g.setColour (juce::Colours::black);
            g.setFont (font);
        }

        // Draw the note text centered in the step
        g.drawText (noteText, stepBounds.toNearestInt(), juce::Justification::centred);
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
        // Calculate which step was clicked
        const int numVisibleSteps = 8;
        const float stepSize = 84.0f;
        const float gap = 16.0f;
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
    const Sequence& sequence; // Reference to the processor's sequence (always live data)
    int activeStepIndex = -1; // -1 means no active step, 0-63 for sequence steps
    int windowStartIndex = 0; // Which step is at the left edge of the window (0-56)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerStepsComponent)
};

