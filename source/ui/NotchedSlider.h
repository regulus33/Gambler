#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * NotchedSlider
 *
 * A reusable rotary slider component with:
 * - A vertical label on the left (rotated 90° counterclockwise)
 * - A 38px hollow knob with white ring
 * - Colored dots around the knob (one per option)
 * - A value gauge below showing the current selection
 *
 * Usage:
 *   NotchedSlider slider ("Note", {"C", "Db", "D", ...}, {red, orange, yellow, ...});
 *   addAndMakeVisible (slider);
 */
class NotchedSlider : public juce::Component {
public:
    /**
     * Constructor
     * @param labelText The text to display vertically on the left (e.g., "Note")
     * @param optionNames Array of strings for each notch (e.g., {"C", "Db", "D", ...})
     * @param optionColours Array of colours for each notch (must match optionNames size)
     */
    NotchedSlider (const juce::String& labelText,
        const juce::Array<juce::String>& optionNames,
        const juce::Array<juce::Colour>& optionColours)
        : labelText (labelText), optionNames (optionNames), optionColours (optionColours) {
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel (&lookAndFeel);
        slider.setRange (0.0, (double) (optionNames.size() - 1), 1.0);
        slider.setValue (0.0);

        addAndMakeVisible (slider);

        slider.onValueChange = [this] { repaint(); };
    }

    ~NotchedSlider() override {
        slider.setLookAndFeel (nullptr);
    }

    juce::Slider& getSlider() { return slider; }
    const juce::Slider& getSlider() const { return slider; }

    int getSelectedIndex() const {
        return (int) std::round (slider.getValue());
    }

    juce::String getSelectedName() const {
        int index = getSelectedIndex();
        if (index >= 0 && index < optionNames.size())
            return optionNames[index];
        return juce::String();
    }

    juce::Colour getSelectedColour() const {
        int index = getSelectedIndex();
        if (index >= 0 && index < optionColours.size())
            return optionColours[index];
        return juce::Colours::white;
    }

    void paint (juce::Graphics& g) override {

        // Draw label on the left
        if (!labelBounds.isEmpty()) {
            juce::Font labelFont;
            if (interFont != nullptr)
                labelFont = juce::Font (interFont);

            labelFont.setHeight (12.0f);
            labelFont.setBold (true);

            g.setFont (labelFont);
            g.setColour (juce::Colours::white);

            juce::Graphics::ScopedSaveState saved (g);
            auto centre = labelBounds.getCentre().toFloat();
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, centre.x, centre.y));
            g.drawText (labelText, labelBounds, juce::Justification::centred, false);
        }

        // Draw value gauge below the slider
        if (!gaugeBounds.isEmpty()) {
            juce::Font gaugeFont;
            if (interFont != nullptr)
                gaugeFont = juce::Font (interFont);

            gaugeFont.setHeight (12.0f);
            gaugeFont.setBold (true);

            auto gaugeColour = getSelectedColour();
            auto r = gaugeBounds.toFloat();

            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (r, 3.0f, 1.0f);

            g.setFont (gaugeFont);
            g.setColour (gaugeColour);

            g.drawText (getSelectedName(), gaugeBounds, juce::Justification::centred, false);
        }
    }

    void resized() override {
        // these are the bounds set by the parent
        auto area = getLocalBounds();

        // Centre the slider in the area
        const int sliderSize = rotarySliderDiameter + 21; // knob diameter + dot ring space
        // create the sliderBounds (remember it's just a rectangle)
        auto sliderBounds = juce::Rectangle<int> (0, 0, sliderSize, sliderSize).withCentre (area.getCentre());
        // now we set the bounds of the juce::Slider itself
        slider.setBounds (sliderBounds);

        // Position label to the left of the slider, 4px away
        // this is the width of the lable before it is transformed and rotated
        const int labelWidth = 28;
        // distance from sliders
        const int labelGap = 24;
        // This is how we figure out where to draw the slider label box
        // rememver all bounds start at x = 0 and y = 0 so subtracting from x moves your box to the left
        // so we get the bounds of the whole juce::Slider area, then move as far left as the gap
        // and as far left as the width
        const int labelX = sliderBounds.getX() - labelGap;
        // figure out how hight to position the label...
        const int labelYOffset = 7; // TODO this needs to come in as an option because we need to change for different sliders OR programmatically center this???
        // now make the label y be offset
        const int labelY = sliderBounds.getCentreY() - labelYOffset;
        // now that we've calculated the label's  size and coordinates we actually create the bounds..
        labelBounds = juce::Rectangle<int> (labelX, labelY, labelWidth, labelWidth);
        // GAUGE
        // all these things start with sizes
        const int gaugeWidth = 28;
        const int gaugeHeight = 16;
        // first we find the vertical center of the slider.
        // sliderBounds.getCentreY.
        // figure out where the center on the vertical access is of this slider, then add slider diameter so we are well below it
        const int sliderVisualBottom = sliderBounds.getCentreY() + (rotarySliderDiameter / 2 + 2);
        // then we create a centered rectangle within this crosshair we paint out with x and y coordinates
        auto gaugeRect = juce::Rectangle<int> (0, 0, gaugeWidth, gaugeHeight);
        // we use that slider visual bottom to calculate the center of the rectangle that makes up the slider
        auto centreY = sliderVisualBottom + 10 + gaugeHeight / 2;
        gaugeBounds = gaugeRect.withCentre (
            { sliderBounds.getCentreX(),
                centreY });
    }

private:
    class LookAndFeel : public juce::LookAndFeel_V4 {
    public:
        LookAndFeel (NotchedSlider* parent) : parent (parent) {}

        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override {
            auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
            auto centre = bounds.getCentre();
            auto radius = juce::jmin (bounds.getWidth() / 2.0f, bounds.getHeight() / 2.0f);

            // Draw hollow knob: 38px diameter with 2px white ring
            const float knobRadius = 19.0f;
            g.setColour (juce::Colours::white);
            g.drawEllipse (centre.getX() - knobRadius, centre.getY() - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 2.0f);

            // Draw coloured dots around the knob at 27px radius
            const int dotCount = parent->optionNames.size();
            const float dotRingRadius = 27.0f;
            const float dotDiameter = 3.0f;

            int currentIndex = (int) std::round (slider.getValue());
            currentIndex = juce::jlimit (0, dotCount - 1, currentIndex);

            for (int i = 0; i < dotCount; ++i) {
                const auto angle = juce::jmap ((float) i / (float) (dotCount - 1),
                    rotaryStartAngle,
                    rotaryEndAngle);
                const auto point = centre.getPointOnCircumference (dotRingRadius, angle);

                float diameter = dotDiameter;
                if (i == currentIndex)
                    diameter = dotDiameter * 2.0f;

                g.setColour (parent->optionColours[i]);
                g.fillEllipse (point.getX() - diameter * 0.5f,
                    point.getY() - diameter * 0.5f,
                    diameter,
                    diameter);
            }

            // Draw white notch pointer
            const auto pointerAngle = rotaryStartAngle
                                      + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
            const auto pointerPoint = centre.getPointOnCircumference (knobRadius - 2.0f, pointerAngle);

            juce::Path pointer;
            pointer.startNewSubPath (pointerPoint);
            pointer.lineTo (centre.getPointOnCircumference (knobRadius * 0.6f, pointerAngle));

            g.setColour (juce::Colours::white);
            g.strokePath (pointer, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

    private:
        NotchedSlider* parent;
    };

    juce::String labelText;
    juce::Array<juce::String> optionNames;
    juce::Array<juce::Colour> optionColours;
    juce::Typeface::Ptr interFont;
    juce::Slider slider;
    LookAndFeel lookAndFeel { this };
    juce::Rectangle<int> labelBounds;
    juce::Rectangle<int> gaugeBounds;
    int rotarySliderDiameter = 38;
};
