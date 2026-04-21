#pragma once

#include "NoteRangeControlsConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Custom LookAndFeel for note-range rotary sliders.
 * White circular track (stroke only), white rectangular thumb on the circumference.
 */
class NoteRangeSliderLookAndFeel : public juce::LookAndFeel_V4 {
public:
    NoteRangeSliderLookAndFeel() = default;

    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider& slider) override {
        juce::ignoreUnused(slider);
        using namespace NoteRangeControlsConstants;

        auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                             static_cast<float>(width), static_cast<float>(height));

        float trackInset = trackStrokeWidth * 0.5f;
        auto trackBounds = bounds.reduced(trackInset);
        float radius = trackBounds.getWidth() * 0.5f;
        auto centre = trackBounds.getCentre();

        g.setColour(juce::Colours::white);
        g.drawEllipse(trackBounds, trackStrokeWidth);

        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        float thumbRadius = radius - (thumbHeight * 0.32f);
        float thumbCentreX = centre.x + thumbRadius * std::sin(angle);
        float thumbCentreY = centre.y - thumbRadius * std::cos(angle);

        juce::Path thumbPath;
        thumbPath.addRectangle(-thumbWidth * 0.5f, -thumbHeight * 0.5f, thumbWidth, thumbHeight);

        juce::AffineTransform transform = juce::AffineTransform::rotation(angle)
                                              .translated(thumbCentreX, thumbCentreY);

        g.setColour(juce::Colours::white);
        g.fillPath(thumbPath, transform);
    }
};
