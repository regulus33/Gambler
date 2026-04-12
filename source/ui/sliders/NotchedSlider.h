#pragma once

#include "../../native/HapticFeedback.h"
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

namespace NotchedSliderConstants {
    // Knob
    constexpr int knobDiameter = 38;
    constexpr float knobRadius = knobDiameter / 2.0f;
    constexpr float knobStrokeWidth = 3.0f;
    constexpr float pointerWidth = 3.0f;
    constexpr int sliderYOffset = 6;
    // Tick marks (radial lines around the knob)
    constexpr float tickRingRadius = 25.0f;      // distance from centre to tick midpoint
    constexpr float tickLength = 2.0f;            // length of each tick
    constexpr float tickWidth = 1.5f;            // stroke width of each tick
    constexpr float tickSelectedLength = 3.0f;   // length of the selected tick
    constexpr float tickSelectedWidth = 2.5f;    // stroke width of the selected tick
    constexpr float tickGap = 6.0f;              // gap between knob circumference and inner tick edge
    // Interaction area: knob + enough space for tick ring + padding
    constexpr int sliderInteractionPadding = static_cast<int>(tickRingRadius * 2) - knobDiameter + 5;
    // Label
    constexpr int labelWidth = 28;
    constexpr int labelGap = 24;
    constexpr int labelYOffset = 7;
    constexpr float labelFontSize = 12.0f;
    // Gauge
    constexpr int gaugeWidth = 28;
    constexpr int gaugeHeight = 16;
    constexpr int gaugeGap = 10;
    constexpr float gaugeFontSize = 12.0f;
}

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
        setPaintingIsUnclipped(true);
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel (&lookAndFeel);
        slider.setScrollWheelEnabled (false);
        slider.setRange (0.0, (double) (optionNames.size() - 1), 1.0);
        slider.setValue (0.0);

        addAndMakeVisible (slider);

        slider.onValueChange = [this] {
            int newIndex = getSelectedIndex();
            if (newIndex != lastNotchIndex_) {
                lastNotchIndex_ = newIndex;
                HapticFeedback::tick();
            }
            repaint();
        };
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

    /**
     * Update the options (names and colours) dynamically
     * This allows changing the available options after construction
     * @param newNames Array of new option names
     * @param newColours Array of new option colours (must match newNames size)
     */
    void updateOptions(const juce::Array<juce::String>& newNames,
                      const juce::Array<juce::Colour>& newColours) {
        if (newNames.isEmpty() || newNames.size() != newColours.size()) {
            return; // Invalid input
        }

        optionNames = newNames;
        optionColours = newColours;

        // Update slider range (ensure end > start for NormalisableRange invariant)
        double rangeEnd = juce::jmax(1.0, (double)(optionNames.size() - 1));
        slider.setRange(0.0, rangeEnd, 1.0);

        // Clamp current value to new range
        double currentValue = slider.getValue();
        if (currentValue >= optionNames.size()) {
            slider.setValue(optionNames.size() - 1, juce::dontSendNotification);
        }

        // Lock slider when there's only one option — no point in dragging
        locked_ = (optionNames.size() == 1);
        if (locked_) {
            jassert(getSelectedIndex() == 0);
            slider.setInterceptsMouseClicks(false, false);
        } else {
            slider.setInterceptsMouseClicks(true, true);
        }

        repaint();
    }

    /** Returns true when the slider has only one option and is locked from interaction. */
    bool isLocked() const { return locked_; }

    void paint (juce::Graphics& g) override {
        using namespace NotchedSliderConstants;

        // Draw label on the left
        if (!labelBounds.isEmpty()) {
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

        // Draw value gauge below the slider
        if (!gaugeBounds.isEmpty()) {
            juce::Font gaugeFont;
            if (interFont != nullptr)
                gaugeFont = juce::Font (interFont);

            gaugeFont.setHeight (gaugeFontSize);
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
        using namespace NotchedSliderConstants;

        auto area = getLocalBounds();

        // Centre the slider in the area
        const int sliderSize = knobDiameter + sliderInteractionPadding;
        auto sliderBounds = juce::Rectangle<int> (0, 0, sliderSize, sliderSize).withCentre (area.getCentre());
        sliderBounds.translate (0, -sliderYOffset);
        slider.setBounds (sliderBounds);

        // Position label to the left of the slider, rotated 90° CCW.
        // drawText clips to the rect, so we use a wide rect (for the text) with
        // narrow height (the visual width after rotation). The rotation around the
        // centre maps width→vertical and height→horizontal.
        const int labelLongSide = area.getHeight();
        const int labelCentreX = sliderBounds.getX() - labelGap + labelWidth / 2;
        const int labelCentreY = sliderBounds.getCentreY();
        labelBounds = juce::Rectangle<int> (
            labelCentreX - labelLongSide / 2,
            labelCentreY - labelWidth / 2,
            labelLongSide,
            labelWidth);

        // Position gauge below the slider
        const int sliderVisualBottom = sliderBounds.getCentreY() + knobDiameter / 2;
        auto gaugeRect = juce::Rectangle<int> (0, 0, gaugeWidth, gaugeHeight);
        auto centreY = sliderVisualBottom + gaugeGap + gaugeHeight / 2;
        gaugeBounds = gaugeRect.withCentre ({ sliderBounds.getCentreX(), centreY });
    }

private:
    class LookAndFeel : public juce::LookAndFeel_V4 {
    public:
        LookAndFeel (NotchedSlider* parent) : parent (parent) {}

        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override {
            using namespace NotchedSliderConstants;
            auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
            auto centre = bounds.getCentre();

            // Draw hollow knob with white ring
            g.setColour (juce::Colours::white);
            g.drawEllipse (centre.getX() - knobRadius, centre.getY() - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, knobStrokeWidth);

            // Draw radial tick marks around the knob
            const int tickCount = parent->optionNames.size();

            int currentIndex = (int) std::round (slider.getValue());
            currentIndex = juce::jlimit (0, tickCount - 1, currentIndex);

            for (int i = 0; i < tickCount; ++i) {
                const float fraction = (tickCount <= 1) ? 0.0f : (float) i / (float) (tickCount - 1);
                const auto angle = juce::jmap (fraction, rotaryStartAngle, rotaryEndAngle);

                bool isSelected = (i == currentIndex);
                float len = isSelected ? tickSelectedLength : tickLength;
                float w   = isSelected ? tickSelectedWidth  : tickWidth;

                float innerR = knobRadius + tickGap;
                auto inner = centre.getPointOnCircumference (innerR, angle);
                auto outer = centre.getPointOnCircumference (innerR + len, angle);

                juce::Path tick;
                tick.startNewSubPath (inner);
                tick.lineTo (outer);

                g.setColour (parent->optionColours[i]);
                g.strokePath (tick, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::square));
            }

            // Draw white notch pointer
            const auto pointerAngle = rotaryStartAngle
                                      + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
            const auto pointerPoint = centre.getPointOnCircumference (knobRadius - 2.0f, pointerAngle);

            juce::Path pointer;
            pointer.startNewSubPath (pointerPoint);
            pointer.lineTo (centre.getPointOnCircumference (knobRadius * 0.6f, pointerAngle));

            g.setColour (juce::Colours::white);
            g.strokePath (pointer, juce::PathStrokeType (pointerWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
    bool locked_ = false;
    int lastNotchIndex_ = 0;
};
