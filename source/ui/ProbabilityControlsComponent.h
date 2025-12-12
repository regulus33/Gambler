#pragma once

#include "AnalogueSlider.h"
#include "ProbabilityGauge.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ProbabilityControlsConstants {
    constexpr float borderStrokeWidth = 2.5f;
    constexpr float cornerRadius = 3.0f;
    constexpr int gaugeSize = 56;
    constexpr int containerHeight = 94;  // Height of the component
}

/**
 * ProbabilityControlsComponent
 *
 * A bordered container for probability-related controls.
 * Displays a 1px white border with 3px corner radius around the controls.
 * Contains an AnalogueSlider (left) and ProbabilityGauge (right) in a 2-column layout.
 */
class ProbabilityControlsComponent : public juce::Component {
public:
    ProbabilityControlsComponent()
        : probabilitySlider ("Probability")
    {
        addAndMakeVisible (probabilitySlider);
        addAndMakeVisible (probabilityGauge);

        // Set up callback for inter-component communication
        probabilitySlider.onValueChange = [this] (float newValue) {
            // Update the gauge when the slider value changes
            probabilityGauge.setProbability (newValue);
        };
    }

    void paint (juce::Graphics& g) override {
        using namespace ProbabilityControlsConstants;
        g.fillAll (juce::Colours::transparentBlack);

        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (r.reduced (0.5f), cornerRadius, borderStrokeWidth);
    }

    void resized() override {
        using namespace ProbabilityControlsConstants;

        auto bounds = getLocalBounds();
        const int halfWidth = bounds.getWidth() / 2;

        // Split into two equal halves
        auto leftHalf = bounds.removeFromLeft (halfWidth);
        auto rightHalf = bounds;

        // Center the slider within the left half (use full height of left half)
        auto sliderArea = leftHalf.withSizeKeepingCentre (leftHalf.getWidth(), leftHalf.getHeight());
        probabilitySlider.setBounds (sliderArea);

        // Center the gauge within the right half
        auto gaugeArea = rightHalf.withSizeKeepingCentre (gaugeSize, gaugeSize);
        probabilityGauge.setBounds (gaugeArea);
    }

    AnalogueSlider& getProbabilitySlider() { return probabilitySlider; }
    const AnalogueSlider& getProbabilitySlider() const { return probabilitySlider; }

    ProbabilityGauge& getProbabilityGauge() { return probabilityGauge; }
    const ProbabilityGauge& getProbabilityGauge() const { return probabilityGauge; }

private:
    AnalogueSlider probabilitySlider;
    ProbabilityGauge probabilityGauge;
};

