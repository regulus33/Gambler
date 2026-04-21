#pragma once

#include "BinaryData.h"
#include "../../backend/MidiLearnRegistry/MidiLearnRegistry.h"
#include "../../backend/MidiMapping/MidiMapping.h"
#include "../MidiLearnOverlayPainter.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * SubdivisionSlider
 *
 * A compact rotary slider for clock subdivision (1/4, 1/8, 1/16, 1/32).
 * Sized to fit the 45px navbar — 36×36px total with a 24px knob.
 * Same visual style as NotchedSlider (white stroke ring, white pointer, colored dots)
 * but without the vertical label or gauge box.
 */

namespace SubdivisionSliderConstants {
    constexpr int knobDiameter = 24;
    constexpr float knobRadius = knobDiameter / 2.0f;
    constexpr float knobStrokeWidth = 2.0f;
    constexpr float pointerWidth = 2.5f;
    constexpr float dotRingRadius = 17.0f;
    constexpr float dotDiameter = 3.0f;
    constexpr float dotSelectedScale = 2.0f;
    constexpr int totalSize = 36;
}

class SubdivisionSlider : public juce::Component {
public:
    SubdivisionSlider() {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(&lf);
        slider.setScrollWheelEnabled(false);
        slider.setRange(0.0, 3.0, 1.0);
        slider.setValue(2.0); // default: 16th notes
        addAndMakeVisible(slider);

        // Add invisible overlay for MIDI learn mode
        addAndMakeVisible(midiLearnOverlay_);
        midiLearnOverlay_.setVisible(false);
        midiLearnOverlay_.setInterceptsMouseClicks(true, false);
        midiLearnOverlay_.onMouseDown = [this]() {
            if (registry_ && registry_->isLearning()) {
                registry_->setLearningControl(MidiControlId::SUBDIVISION_SLIDER);
                if (onMidiLearnSelected) onMidiLearnSelected();
            }
        };

        setSize(SubdivisionSliderConstants::totalSize,
                SubdivisionSliderConstants::totalSize);
    }

    ~SubdivisionSlider() override {
        slider.setLookAndFeel(nullptr);
    }

    juce::Slider& getSlider() { return slider; }

    void setRegistry(MidiLearnRegistry* registry) { registry_ = registry; }

    std::function<void()> onMidiLearnSelected;

    void paint(juce::Graphics& g) override {
        // Paint MIDI learn overlay on top
        bool isLearning = registry_ && registry_->isLearning();
        midiLearnOverlay_.setVisible(isLearning);
        if (isLearning) {
            MidiLearnOverlayPainter::paint(g, getLocalBounds(),
                                           registry_, MidiControlId::SUBDIVISION_SLIDER, 0.0f);
        }
    }

    void resized() override {
        slider.setBounds(getLocalBounds());
        midiLearnOverlay_.setBounds(getLocalBounds());
    }

private:
    // Transparent overlay for MIDI learn click interception
    class MidiLearnOverlay : public juce::Component {
    public:
        std::function<void()> onMouseDown;
        void mouseDown(const juce::MouseEvent&) override {
            if (onMouseDown) onMouseDown();
        }
        void paint(juce::Graphics&) override {}
    };

    static constexpr int numNotches = 4;

    class LookAndFeel : public juce::LookAndFeel_V4 {
    public:
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                              float sliderPos, float startAngle, float endAngle,
                              juce::Slider& s) override
        {
            using namespace SubdivisionSliderConstants;

            auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
            auto centre = bounds.getCentre();

            // Scale everything relative to actual bounds vs design size
            float scale = juce::jmin(bounds.getWidth(), bounds.getHeight()) / static_cast<float>(totalSize);
            float scaledKnobRadius = knobRadius * scale;
            float scaledKnobDiameter = knobDiameter * scale;
            float scaledDotRingRadius = dotRingRadius * scale;
            float scaledDotDiameter = dotDiameter * scale;
            float scaledStrokeWidth = knobStrokeWidth * scale;
            float scaledPointerWidth = pointerWidth * scale;

            // Hollow knob ring
            g.setColour(juce::Colours::white);
            g.drawEllipse(centre.getX() - scaledKnobRadius,
                          centre.getY() - scaledKnobRadius,
                          scaledKnobDiameter, scaledKnobDiameter,
                          scaledStrokeWidth);

            int currentIndex = juce::jlimit(0, numNotches - 1,
                                            (int) std::round(s.getValue()));

            // Dots
            for (int i = 0; i < numNotches; ++i) {
                float angle = juce::jmap((float) i / (float) (numNotches - 1),
                                         startAngle, endAngle);
                auto pt = centre.getPointOnCircumference(scaledDotRingRadius, angle);
                float d = (i == currentIndex) ? scaledDotDiameter * dotSelectedScale : scaledDotDiameter;

                g.setColour(juce::Colours::white);
                g.fillEllipse(pt.getX() - d * 0.5f,
                              pt.getY() - d * 0.5f,
                              d, d);
            }

            // White pointer
            float pointerAngle = startAngle + sliderPos * (endAngle - startAngle);
            auto outer = centre.getPointOnCircumference(scaledKnobRadius - 1.5f * scale, pointerAngle);
            auto inner = centre.getPointOnCircumference(scaledKnobRadius * 0.55f, pointerAngle);

            juce::Path pointer;
            pointer.startNewSubPath(outer);
            pointer.lineTo(inner);

            g.setColour(juce::Colours::white);
            g.strokePath(pointer, juce::PathStrokeType(scaledPointerWidth,
                juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    };

    juce::Slider slider;
    LookAndFeel lf;
    MidiLearnOverlay midiLearnOverlay_;
    MidiLearnRegistry* registry_ = nullptr;
};
