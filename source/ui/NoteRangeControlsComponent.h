#pragma once

#include "BinaryData.h"
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
 */
class NoteRangeGauge : public juce::Component {
public:
    NoteRangeGauge() = default;

    void setRange (int newMin, int newMax) {
        minNote = newMin;
        maxNote = newMax;
        repaint();
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

            // Color based on whether note is in range
            if (noteIndex >= minNote && noteIndex <= maxNote)
                g.setColour (juce::Colour (rangeActiveColourHex));
            else
                g.setColour (juce::Colour (rangeInactiveColourHex));

            g.fillRect (noteRect);
        }
    }

private:
    int minNote = 0;
    int maxNote = 127;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteRangeGauge)
};

/**
 * NoteRangeControlsComponent
 *
 * Panel displayed to the right of the sequencer steps/controls.
 * Contains controls for note range settings (min/max range sliders).
 */
class NoteRangeControlsComponent : public juce::Component {
public:
    NoteRangeControlsComponent() {
        // Load Inter 24pt Bold static font (designed for medium display sizes)
        interBoldFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::Inter_24ptBold_ttf,
            BinaryData::Inter_24ptBold_ttfSize);

        // Setup min range slider
        minRangeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        minRangeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        minRangeSlider.setRange (0.0, 127.0, 1.0);
        minRangeSlider.setValue (0.0);
        minRangeSlider.setLookAndFeel (&sliderLookAndFeel);
        minRangeSlider.onValueChange = [this] { updateGaugeRange(); };
        addAndMakeVisible (minRangeSlider);

        // Setup max range slider
        maxRangeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        maxRangeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        maxRangeSlider.setRange (0.0, 127.0, 1.0);
        maxRangeSlider.setValue (127.0);
        maxRangeSlider.setLookAndFeel (&sliderLookAndFeel);
        maxRangeSlider.onValueChange = [this] { updateGaugeRange(); };
        addAndMakeVisible (maxRangeSlider);

        // Setup range gauge
        addAndMakeVisible (rangeGauge);
    }

    ~NoteRangeControlsComponent() override {
        minRangeSlider.setLookAndFeel (nullptr);
        maxRangeSlider.setLookAndFeel (nullptr);
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
        // Calculate the available space: from top to just above maxRangeSlider
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

        // Position MinRangeSlider at bottom-left with padding
        int minSliderX = sliderPadding + contentPaddingLeft;
        int minSliderY = getHeight() - sliderPadding - sliderDiameter;
        minRangeSlider.setBounds (minSliderX, minSliderY, sliderDiameter, sliderDiameter);

        // Position MaxRangeSlider directly above MinRangeSlider with vertical spacing
        int maxSliderX = sliderPadding + contentPaddingLeft;
        int maxSliderY = minSliderY - sliderVerticalSpacing - sliderDiameter;
        maxRangeSlider.setBounds (maxSliderX, maxSliderY, sliderDiameter, sliderDiameter);

        // Position range gauge in the right half with margins
        int halfWidth = getWidth() / 2;
        int gaugeX = halfWidth + rangeBarHorizontalMargin + rangeBarXOffset;
        int gaugeY = rangeBarVerticalMargin;
        int gaugeWidth = halfWidth - (rangeBarHorizontalMargin * 2);
        int gaugeHeight = getHeight() - (rangeBarVerticalMargin * 2);
        rangeGauge.setBounds (gaugeX, gaugeY, gaugeWidth, gaugeHeight);
    }

private:
    void updateGaugeRange() {
        int minNote = static_cast<int>(minRangeSlider.getValue());
        int maxNote = static_cast<int>(maxRangeSlider.getValue());
        rangeGauge.setRange (minNote, maxNote);
    }


    juce::Typeface::Ptr interBoldFont;
    NoteRangeSliderLookAndFeel sliderLookAndFeel;
    juce::Slider minRangeSlider;
    juce::Slider maxRangeSlider;
    NoteRangeGauge rangeGauge;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteRangeControlsComponent)
};

