#pragma once

#include "BinaryData.h"
#include "StateListenerComponent.h"
#include "../backend/Scale.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace NoteRangeControlsConstants {
    // Border
    constexpr float borderWidth = 2.0f;
    constexpr float borderRadius = 3.0f;
    // Content padding (left side - sliders and label)
    constexpr int contentPaddingLeft = 4;   // Extra padding from left edge
    // Sliders
    constexpr int sliderDiameter = 25;
    constexpr int sliderPadding = 8;
    constexpr int sliderVerticalSpacing = 6;
    // Slider appearance
    constexpr float trackStrokeWidth = 3.0f;
    constexpr float thumbWidth = 2.0f;
    constexpr float thumbHeight = 8.0f;
    // Range bar (visual representation)
    constexpr int rangeBarHorizontalMargin = 12;
    constexpr int rangeBarVerticalMargin = 12;
    constexpr int rangeBarXOffset = -4;      // Shift gauge left (negative) or right (positive) (rightmost pasdding)
    constexpr int midiNoteCount = 128;
    // Range bar colours
    constexpr juce::uint32 rangeActiveColourHex = 0xFF4CAF50;   // Green for notes in range
    constexpr juce::uint32 rangeInactiveColourHex = 0xFF373737; // Grey for notes out of range
    // Label
    constexpr float labelFontSize = 22.0f;
    constexpr int labelGapFromSlider = 4;
}

/**
 * Custom LookAndFeel for note range rotary sliders
 * - White circular track (stroke only)
 * - White rectangular thumb on the circumference
 */
class NoteRangeSliderLookAndFeel : public juce::LookAndFeel_V4 {
public:
    NoteRangeSliderLookAndFeel() = default;

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        juce::ignoreUnused (slider);
        using namespace NoteRangeControlsConstants;

        auto bounds = juce::Rectangle<float> (static_cast<float>(x), static_cast<float>(y),
                                               static_cast<float>(width), static_cast<float>(height));

        // Calculate the track circle dimensions (inset by half the stroke width)
        float trackInset = trackStrokeWidth * 0.5f;
        auto trackBounds = bounds.reduced (trackInset);
        float radius = trackBounds.getWidth() * 0.5f;
        auto centre = trackBounds.getCentre();

        // Draw the circular track (white stroke, no fill)
        g.setColour (juce::Colours::white);
        g.drawEllipse (trackBounds, trackStrokeWidth);

        // Calculate the thumb position on the circumference
        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        // Position thumb center slightly inside the circumference so it doesn't extend outside
        float thumbRadius = radius - (thumbHeight * 0.32f);
        float thumbCentreX = centre.x + thumbRadius * std::sin (angle);
        float thumbCentreY = centre.y - thumbRadius * std::cos (angle);

        // Draw the thumb as a rotated rectangle
        juce::Path thumbPath;
        thumbPath.addRectangle (-thumbWidth * 0.5f, -thumbHeight * 0.5f, thumbWidth, thumbHeight);

        // Rotate the thumb to be tangent to the circle (perpendicular to radius)
        juce::AffineTransform transform = juce::AffineTransform::rotation (angle)
                                              .translated (thumbCentreX, thumbCentreY);

        g.setColour (juce::Colours::white);
        g.fillPath (thumbPath, transform);
    }
};

/**
 * NoteRangeGauge
 *
 * Visual representation of the MIDI note range.
 * Displays 128 rectangles (one per MIDI note), colored based on min/max range.
 * The currently playing note is highlighted in white.
 */
class NoteRangeGauge : public juce::Component {
public:
    NoteRangeGauge() = default;

    void setRange (int newMin, int newMax) {
        minNote = newMin;
        maxNote = newMax;
        repaint();
    }

    void setActiveNote (int noteNumber) {
        if (activeNote != noteNumber) {
            activeNote = noteNumber;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override {
        using namespace NoteRangeControlsConstants;

        auto bounds = getLocalBounds().toFloat();

        // Draw 128 rectangles (one per MIDI note), stacked vertically
        float noteHeight = bounds.getHeight() / static_cast<float>(midiNoteCount);
        float noteWidth = bounds.getWidth();

        for (int i = 0; i < midiNoteCount; ++i) {
            // Draw from top (note 127) to bottom (note 0)
            int noteIndex = midiNoteCount - 1 - i;
            float y = bounds.getY() + static_cast<float>(i) * noteHeight;

            juce::Rectangle<float> noteRect (bounds.getX(), y, noteWidth, noteHeight);

            // Color: white for active note, green for in-range, grey for out-of-range
            if (noteIndex == activeNote)
                g.setColour (juce::Colours::white);
            else if (noteIndex >= minNote && noteIndex <= maxNote)
                g.setColour (juce::Colour (rangeActiveColourHex));
            else
                g.setColour (juce::Colour (rangeInactiveColourHex));

            g.fillRect (noteRect);
        }
    }

private:
    int minNote = 0;
    int maxNote = 127;
    int activeNote = -1;  // -1 means no active note

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteRangeGauge)
};

/**
 * NoteRangeControlsComponent
 *
 * Panel displayed to the right of the sequencer steps/controls.
 * Contains controls for note range settings using a window-based approach:
 * - Window Size (bottom slider): How many notes are in the valid range (1-128)
 * - Window Offset (top slider): Starting position of the window (0-127)
 *
 * Example: size=45, offset=54 → valid notes are [54, 98] (54 + 45 - 1)
 *
 * Observer Pattern:
 * - Listens to scale window parameters (offset, size)
 * - Automatically updates UI when window parameters change
 * - Directly maps to backend window parameters (no conversion needed)
 */
class NoteRangeControlsComponent : public StateListenerComponent {
public:
    explicit NoteRangeControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor) {
        // Load Inter 24pt Bold static font (designed for medium display sizes)
        interBoldFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::Inter_24ptBold_ttf,
            BinaryData::Inter_24ptBold_ttfSize);

        // Setup window width slider (bottom slider)
        // Range: 1 to 128 (minimum window width is 1 note)
        windowWidthSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        windowWidthSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        windowWidthSlider.setRange (1.0, 128.0, 1.0);
        windowWidthSlider.setValue (128.0);  // Default: full range
        windowWidthSlider.setLookAndFeel (&sliderLookAndFeel);
        windowWidthSlider.onValueChange = [this] {
            clampAndUpdateValues();
            updateBackendAndGauge();
        };
        addAndMakeVisible (windowWidthSlider);

        // Setup window offset slider (top slider)
        // Range: 0 to 127 (starting position of the window)
        windowOffsetSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        windowOffsetSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        windowOffsetSlider.setRange (0.0, 127.0, 1.0);
        windowOffsetSlider.setValue (0.0);  // Default: start at note 0
        windowOffsetSlider.setLookAndFeel (&sliderLookAndFeel);
        windowOffsetSlider.onValueChange = [this] {
            clampAndUpdateValues();
            updateBackendAndGauge();
        };
        addAndMakeVisible (windowOffsetSlider);

        // Setup range gauge
        addAndMakeVisible (rangeGauge);

        // Subscribe to scale range parameters
        listenToScaleRange();
    }

    /**
     * Set the current window values (for initialization from backend)
     */
    void setWindow(int offset, int size) {
        windowOffsetSlider.setValue(static_cast<double>(offset), juce::dontSendNotification);
        windowWidthSlider.setValue(static_cast<double>(size), juce::dontSendNotification);

        // Update gauge with computed min/max
        int minNote = Scale::computeMinFromWindow(offset, size);
        int maxNote = Scale::computeMaxFromWindow(offset, size);
        rangeGauge.setRange(minNote, maxNote);
    }

    /**
     * Set the currently playing note (highlighted in white on the gauge)
     */
    void setActiveNote(int noteNumber) {
        rangeGauge.setActiveNote(noteNumber);
    }

    ~NoteRangeControlsComponent() override {
        windowWidthSlider.setLookAndFeel (nullptr);
        windowOffsetSlider.setLookAndFeel (nullptr);
    }

    void paint (juce::Graphics& g) override {
        using namespace NoteRangeControlsConstants;

        auto bounds = getLocalBounds().toFloat();

        // Draw white border inside the bounds
        g.setColour (juce::Colours::white);
        float inset = borderWidth * 0.5f;
        juce::Rectangle<float> borderBounds = bounds.reduced (inset);
        g.drawRoundedRectangle (borderBounds, borderRadius, borderWidth);

        // Draw rotated "Note Range" label in the left half, above the sliders
        // Calculate the available space: from top to just above windowOffsetSlider (top slider)
        float labelAreaTop = static_cast<float>(sliderPadding);
        float labelAreaBottom = static_cast<float>(getHeight() - sliderPadding - sliderDiameter - sliderVerticalSpacing - sliderDiameter - labelGapFromSlider);
        float labelAreaHeight = labelAreaBottom - labelAreaTop;
        float labelAreaCentreX = static_cast<float>(sliderPadding + contentPaddingLeft + sliderDiameter / 2);
        float labelAreaCentreY = labelAreaTop + labelAreaHeight / 2.0f;

        g.setColour (juce::Colours::white);

        juce::Font labelFont;
        if (interBoldFont != nullptr)
            labelFont = juce::Font (interBoldFont);
        labelFont.setHeight (labelFontSize);
        g.setFont (labelFont);

        // Save the current transform, rotate 90° counter-clockwise, draw text, restore
        g.saveState();
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                                                          labelAreaCentreX,
                                                          labelAreaCentreY));

        // Draw text centered at the rotation point (text will now read bottom-to-top)
        juce::Rectangle<float> textBounds (labelAreaCentreX - labelAreaHeight / 2.0f,
                                            labelAreaCentreY - labelFontSize / 2.0f,
                                            labelAreaHeight,
                                            labelFontSize);
        g.drawText ("Note Range", textBounds, juce::Justification::centred, false);
        g.restoreState();
    }

    void resized() override {
        using namespace NoteRangeControlsConstants;

        // Position WindowWidthSlider at bottom-left with padding
        int widthSliderX = sliderPadding + contentPaddingLeft;
        int widthSliderY = getHeight() - sliderPadding - sliderDiameter;
        windowWidthSlider.setBounds (widthSliderX, widthSliderY, sliderDiameter, sliderDiameter);

        // Position WindowOffsetSlider directly above WindowWidthSlider with vertical spacing
        int offsetSliderX = sliderPadding + contentPaddingLeft;
        int offsetSliderY = widthSliderY - sliderVerticalSpacing - sliderDiameter;
        windowOffsetSlider.setBounds (offsetSliderX, offsetSliderY, sliderDiameter, sliderDiameter);

        // Position range gauge in the right half with margins
        int halfWidth = getWidth() / 2;
        int gaugeX = halfWidth + rangeBarHorizontalMargin + rangeBarXOffset;
        int gaugeY = rangeBarVerticalMargin;
        int gaugeWidth = halfWidth - (rangeBarHorizontalMargin * 2);
        int gaugeHeight = getHeight() - (rangeBarVerticalMargin * 2);
        rangeGauge.setBounds (gaugeX, gaugeY, gaugeWidth, gaugeHeight);
    }

protected:
    /**
     * Called on message thread when any subscribed parameter changes.
     * Updates the sliders and gauge to reflect the current window state.
     */
    void onStateChanged() override {
        auto& state = getState();
        int offset = state.getScaleWindowOffset();
        int size = state.getScaleWindowSize();

        // Update sliders directly from window parameters
        windowOffsetSlider.setValue(static_cast<double>(offset), juce::dontSendNotification);
        windowWidthSlider.setValue(static_cast<double>(size), juce::dontSendNotification);

        // Update gauge with computed min/max
        int minNote = Scale::computeMinFromWindow(offset, size);
        int maxNote = Scale::computeMaxFromWindow(offset, size);
        rangeGauge.setRange(minNote, maxNote);

        // Force immediate repaint of the gauge to ensure visual feedback
        rangeGauge.repaint();
    }

private:
    /**
     * Clamp slider values to ensure offset + width - 1 <= 127
     * Called whenever either slider changes
     * Keeps slider ranges static but clamps the actual values
     */
    void clampAndUpdateValues() {
        constexpr int MAX_MIDI_NOTE = 127;

        int currentOffset = static_cast<int>(windowOffsetSlider.getValue());
        int currentWidth = static_cast<int>(windowWidthSlider.getValue());

        // Calculate maximum allowed offset for current width
        // If width is 20, max offset is 127 - 20 + 1 = 108
        // This ensures offset + width - 1 <= 127
        int maxAllowedOffset = MAX_MIDI_NOTE - currentWidth + 1;

        // Clamp offset if it exceeds the limit
        if (currentOffset > maxAllowedOffset) {
            windowOffsetSlider.setValue(static_cast<double>(maxAllowedOffset), juce::dontSendNotification);
        }
    }

    void updateBackendAndGauge() {
        // Get current window values from sliders
        int offset = static_cast<int>(windowOffsetSlider.getValue());
        int size = static_cast<int>(windowWidthSlider.getValue());

        // Compute min/max for gauge display
        int minNote = Scale::computeMinFromWindow(offset, size);
        int maxNote = Scale::computeMaxFromWindow(offset, size);

        // Update gauge
        rangeGauge.setRange(minNote, maxNote);

        // Update backend parameters directly
        auto& state = getState();
        state.setScaleWindowOffset(offset);
        state.setScaleWindowSize(size);
    }


    juce::Typeface::Ptr interBoldFont;
    NoteRangeSliderLookAndFeel sliderLookAndFeel;
    juce::Slider windowWidthSlider;   // Bottom slider: window width (1-128)
    juce::Slider windowOffsetSlider;  // Top slider: window offset (0-127)
    NoteRangeGauge rangeGauge;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteRangeControlsComponent)
};

