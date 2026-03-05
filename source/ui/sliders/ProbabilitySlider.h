#pragma once

#include "../../backend/MidiLearnRegistry.h"
#include "../../backend/MidiMapping.h"
#include "../MidiLearnOverlayPainter.h"
#include "BinaryData.h"
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

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
    constexpr float knobStrokeWidth = 3.0f;
    constexpr float thumbWidth = 3.0f;
    constexpr int sliderYOffset = 6;
    constexpr int sliderInteractionPadding = 21;  // extra space around knob for interaction
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
        : labelText (labelText),
          midiControlId_(MidiControlId::PROBABILITY_SLIDER) {
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel (&lookAndFeel);
        slider.setRange (0.0, 1.0, 0.0); // Continuous range 0.0 to 1.0
        slider.setValue (0.0);

        addAndMakeVisible (slider);

        // Add invisible overlay for MIDI learn mode (sits on top, initially hidden)
        addAndMakeVisible(midiLearnOverlay_);
        midiLearnOverlay_.setVisible(false);
        midiLearnOverlay_.setInterceptsMouseClicks(true, false);
        midiLearnOverlay_.onMouseDown = [this]() {
            if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
                DBG("AnalogueSlider overlay clicked: Setting learning control to " << static_cast<int>(midiControlId_));
                registry_->setLearningControl(midiControlId_);
                DBG("AnalogueSlider: Learning control is now " << static_cast<int>(registry_->getLearningControl()));

                // Notify that this control was selected (triggers repaint of all MIDI learn components)
                if (onMidiLearnSelected) {
                    onMidiLearnSelected();
                }
            }
        };

        slider.onValueChange = [this] {
            repaint();
            if (onValueChange)
                onValueChange (static_cast<float> (slider.getValue()));
        };
    }

    ~AnalogueSlider() override {
        slider.setLookAndFeel (nullptr);
    }

    /**
     * Set the MIDI learn registry for this slider
     */
    void setMidiLearnRegistry(MidiLearnRegistry* registry) {
        registry_ = registry;
    }

    /**
     * Callback when this control is selected for MIDI learning
     * Use this to trigger repainting of all MIDI learn components
     */
    std::function<void()> onMidiLearnSelected;

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

        // Paint MIDI learn overlay if in learn mode
        if (registry_ && registry_->isLearning()) {
            bool isLearningThis = (registry_->getLearningControl() == midiControlId_);
            DBG("AnalogueSlider paint: isLearning=" << (int)registry_->isLearning()
                << " isLearningThis=" << (int)isLearningThis
                << " controlId=" << static_cast<int>(midiControlId_));

            // Show the overlay when in learn mode
            midiLearnOverlay_.setVisible(true);
        } else {
            // Hide the overlay when not in learn mode
            midiLearnOverlay_.setVisible(false);
        }

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

        // Paint MIDI learn overlay border
        paintMidiLearnOverlay(g);
    }

    void resized() override {
        using namespace AnalogueSliderConstants;

        auto area = getLocalBounds();

        // Centre the slider in the area
        const int sliderSize = knobDiameter + sliderInteractionPadding;
        auto sliderBounds = juce::Rectangle<int> (0, 0, sliderSize, sliderSize).withCentre (area.getCentre());
        sliderBounds.translate (0, -sliderYOffset);
        slider.setBounds (sliderBounds);

        // Position label to the left of the slider
        const int labelX = sliderBounds.getX() - labelGap;
        const int labelY = sliderBounds.getCentreY() - labelYOffset;
        labelBounds = juce::Rectangle<int> (labelX, labelY, labelWidth, labelWidth);

        // Position gauge below the slider
        const int sliderVisualBottom = sliderBounds.getCentreY() + knobDiameter / 2;
        auto gaugeRect = juce::Rectangle<int> (0, 0, gaugeWidth, gaugeHeight);
        auto centreY = sliderVisualBottom + gaugeGap + gaugeHeight / 2;
        gaugeBounds = gaugeRect.withCentre ({ sliderBounds.getCentreX(), centreY });

        // Position MIDI learn overlay to cover the entire component
        midiLearnOverlay_.setBounds(getLocalBounds());
    }

private:
    void paintMidiLearnOverlay(juce::Graphics& g) {
        // No corner radius for sliders (they're circular knobs)
        MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, 0.0f);
    }

    // Transparent overlay component that sits on top of the slider in MIDI learn mode
    class MidiLearnOverlay : public juce::Component {
    public:
        std::function<void()> onMouseDown;

        void mouseDown(const juce::MouseEvent&) override {
            if (onMouseDown) {
                onMouseDown();
            }
        }

        void paint(juce::Graphics&) override {
            // Transparent - the parent will paint the overlay visuals
        }
    };

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
    MidiLearnOverlay midiLearnOverlay_;
    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;
};

