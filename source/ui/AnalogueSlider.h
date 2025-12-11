#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * AnalogueSlider
 *
 * A continuous rotary slider component with:
 * - A vertical label on the left (rotated 90° counterclockwise, reading bottom to top)
 * - A hollow knob with white ring
 * - A value indicator line showing current position
 * - Percentage display below showing the current value (0% to 100%)
 *
 * Usage:
 *   AnalogueSlider slider ("Prob");
 *   slider.onValueChange = [](float newValue) { ... };
 *   addAndMakeVisible (slider);
 */

// Sizing constants - adjust these to experiment with different sizes
namespace AnalogueSliderConstants {
    constexpr int knobDiameter = 38;
    constexpr float knobRadius = knobDiameter / 2.0f;
    constexpr float knobStrokeWidth = 3.0f; // change width of stroke around
    constexpr float thumbWidth = 3.0f;
    constexpr int sliderYOffset = 6;
    constexpr int labelWidth = 28;
    constexpr int labelGap = 18;
    constexpr int labelYOffset = 7;
    constexpr float labelFontSize = 12.0f;
    constexpr int gaugeWidth = 42;
    constexpr int gaugeHeight = 16;
    constexpr int gaugeGap = 10;
    constexpr float gaugeFontSize = 12.0f;
    constexpr float percentageFontSize = 10.0f;
}

class AnalogueSlider : public juce::Component {
public:
    /**
     * Constructor
     * @param labelText The text to display vertically on the left (e.g., "Prob")
     */
    explicit AnalogueSlider (const juce::String& labelText)
        : labelText (labelText) {
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel (&lookAndFeel);
        slider.setRange (0.0, 1.0, 0.0); // Continuous range 0.0 to 1.0
        slider.setValue (0.0);

        addAndMakeVisible (slider);

        slider.onValueChange = [this] {
            repaint();
            if (onValueChange)
                onValueChange (static_cast<float> (slider.getValue()));
        };
    }

    ~AnalogueSlider() override {
        slider.setLookAndFeel (nullptr);
    }

    juce::Slider& getSlider() { return slider; }
    const juce::Slider& getSlider() const { return slider; }

    float getValue() const {
        return static_cast<float> (slider.getValue());
    }

    void setValue (float newValue, juce::NotificationType notification = juce::sendNotificationAsync) {
        slider.setValue (newValue, notification);
    }

    int getPercentage() const {
        return static_cast<int> (std::round (slider.getValue() * 100.0));
    }

    juce::String getPercentageString() const {
        return juce::String (getPercentage()) + "%";
    }

    // Callback for value changes - set this from outside to receive notifications
    std::function<void (float)> onValueChange;

    void paint (juce::Graphics& g) override {
        using namespace AnalogueSliderConstants;

        // Draw label on the left (rotated 90° counterclockwise - bottom to top)
        if (! labelBounds.isEmpty()) {
            juce::Font labelFont;
            if (interFont != nullptr)
                labelFont = juce::Font (interFont);

            labelFont.setHeight (labelFontSize);
            labelFont.setBold (true);

            g.setFont (labelFont);
            g.setColour (juce::Colours::white);

            juce::Graphics::ScopedSaveState saved (g);
            auto centre = labelBounds.getCentre().toFloat();
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, centre.x, centre.y));
            g.drawText (labelText, labelBounds, juce::Justification::centred, false);
        }

        // Draw percentage gauge below the slider
        if (! gaugeBounds.isEmpty()) {
            juce::Font gaugeFont;
            if (interFont != nullptr)
                gaugeFont = juce::Font (interFont);

            gaugeFont.setHeight (gaugeFontSize);
            gaugeFont.setBold (true);

            auto r = gaugeBounds.toFloat();

            // White border around gauge
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (r, 3.0f, 1.0f);

            // Percentage text in salmon/coral color (matching the reference image)
            g.setFont (gaugeFont);
            g.setColour (juce::Colour (0xFFE57373)); // Salmon/coral color
            g.drawText (getPercentageString(), gaugeBounds, juce::Justification::centred, false);
        }
    }

    void resized() override {
        using namespace AnalogueSliderConstants;

        auto area = getLocalBounds();

        // Centre the slider in the area
        const int sliderSize = knobDiameter + 21; // knob diameter + some space for interaction
        auto sliderBounds = juce::Rectangle<int> (0, 0, sliderSize, sliderSize).withCentre (area.getCentre());
        sliderBounds.translate (0, -sliderYOffset);
        slider.setBounds (sliderBounds);

        // Position label to the left of the slider
        const int labelX = sliderBounds.getX() - labelGap;
        const int labelY = sliderBounds.getCentreY() - labelYOffset;
        labelBounds = juce::Rectangle<int> (labelX, labelY, labelWidth, labelWidth);

        // Position gauge below the slider
        const int sliderVisualBottom = sliderBounds.getCentreY() + (knobDiameter / 2 + 2) - 2;
        auto gaugeRect = juce::Rectangle<int> (0, 0, gaugeWidth, gaugeHeight);
        auto centreY = sliderVisualBottom + gaugeGap + gaugeHeight / 2;
        gaugeBounds = gaugeRect.withCentre ({ sliderBounds.getCentreX(), centreY });
    }

private:
    class LookAndFeel : public juce::LookAndFeel_V4 {
    public:
        explicit LookAndFeel (AnalogueSlider* parent) : parent (parent) {}

        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPosProportional, float rotaryStartAngle,
                               float rotaryEndAngle, juce::Slider& /*slider*/) override {
            using namespace AnalogueSliderConstants;

            auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
            auto centre = bounds.getCentre();

            // Draw hollow knob: white ring
            g.setColour (juce::Colours::white);
            g.drawEllipse (centre.getX() - knobRadius,
                           centre.getY() - knobRadius,
                           knobRadius * 2.0f,
                           knobRadius * 2.0f,
                           knobStrokeWidth);

            // Draw value indicator pointer
            const auto pointerAngle = rotaryStartAngle
                                      + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
            const auto pointerPoint = centre.getPointOnCircumference (knobRadius - 2.0f, pointerAngle);

            juce::Path pointer;
            pointer.startNewSubPath (pointerPoint);
            pointer.lineTo (centre.getPointOnCircumference (knobRadius * 0.6f, pointerAngle));

            g.setColour (juce::Colours::white);
            g.strokePath (pointer, juce::PathStrokeType (thumbWidth, juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }

    private:
        AnalogueSlider* parent;
    };

    juce::String labelText;
    juce::Typeface::Ptr interFont;
    juce::Slider slider;
    LookAndFeel lookAndFeel { this };
    juce::Rectangle<int> labelBounds;
    juce::Rectangle<int> gaugeBounds;
};

