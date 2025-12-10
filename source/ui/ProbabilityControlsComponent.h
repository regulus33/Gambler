#pragma once

#include "AnalogueSlider.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * ProbabilityControlsComponent
 *
 * A bordered container for probability-related controls.
 * Displays a 1px white border with 3px corner radius around the controls.
 */
class ProbabilityControlsComponent : public juce::Component {
public:
    ProbabilityControlsComponent()
        : probabilitySlider ("Probability")
    {
        addAndMakeVisible (probabilitySlider);

        // Set up callback for inter-component communication
        probabilitySlider.onValueChange = [this] (float newValue) {
            // TODO: Update other probability-related components when slider value changes
            juce::ignoreUnused (newValue);
        };
    }

    void paint (juce::Graphics& g) override {
        g.fillAll (juce::Colours::transparentBlack);

        // 1px white border with 3px corner radius
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    }

    void resized() override {
        constexpr int innerPadding = 4;

        // Layout area inside the container
        auto contentArea = getLocalBounds().reduced (innerPadding);

        // For now, center the single AnalogueSlider within the container
        // Leave room in the layout logic for additional components to be added later
        probabilitySlider.setBounds (contentArea);
    }

    AnalogueSlider& getProbabilitySlider() { return probabilitySlider; }
    const AnalogueSlider& getProbabilitySlider() const { return probabilitySlider; }

private:
    AnalogueSlider probabilitySlider;
};

