#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace NoteRangeControlsConstants {
        // Border
    constexpr float borderWidth = 3.0f;
    constexpr float borderRadius = 3.0f;
    // Sliders
    constexpr int sliderDiameter = 25;
    constexpr int sliderPadding = 8;
    constexpr int sliderVerticalSpacing = 6;
    // Slider appearance
    constexpr float trackStrokeWidth = 3.0f;
    constexpr float thumbWidth = 2.0f;
    constexpr float thumbHeight = 8.0f;
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
 * NoteRangeControlsComponent
 *
 * Panel displayed to the right of the sequencer steps/controls.
 * Contains controls for note range settings (min/max range sliders).
 */
class NoteRangeControlsComponent : public juce::Component {
public:
    NoteRangeControlsComponent() {
        // Setup min range slider
        minRangeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        minRangeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        minRangeSlider.setLookAndFeel (&sliderLookAndFeel);
        addAndMakeVisible (minRangeSlider);

        // Setup max range slider
        maxRangeSlider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        maxRangeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        maxRangeSlider.setLookAndFeel (&sliderLookAndFeel);
        addAndMakeVisible (maxRangeSlider);
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
    }

    void resized() override {
        using namespace NoteRangeControlsConstants;

        // Position MinRangeSlider at bottom-left with padding
        int minSliderX = sliderPadding;
        int minSliderY = getHeight() - sliderPadding - sliderDiameter;
        minRangeSlider.setBounds (minSliderX, minSliderY, sliderDiameter, sliderDiameter);

        // Position MaxRangeSlider directly above MinRangeSlider with vertical spacing
        int maxSliderX = sliderPadding;
        int maxSliderY = minSliderY - sliderVerticalSpacing - sliderDiameter;
        maxRangeSlider.setBounds (maxSliderX, maxSliderY, sliderDiameter, sliderDiameter);
    }

private:
    NoteRangeSliderLookAndFeel sliderLookAndFeel;
    juce::Slider minRangeSlider;
    juce::Slider maxRangeSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteRangeControlsComponent)
};

