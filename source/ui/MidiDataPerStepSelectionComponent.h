#pragma once

#include "VerticalBarSlider.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MidiDataPerStepSelectionComponent
 *
 * Displays MIDI CC parameter selection for each visible step.
 * Size: 408 x 199 pixels (matches PianoRollComponent)
 *
 * Currently shows a single vertical bar slider for development.
 */
class MidiDataPerStepSelectionComponent : public juce::Component {
public:
    MidiDataPerStepSelectionComponent() {
        // Create a single slider for now
        slider = std::make_unique<VerticalBarSlider>(50, 150);
        addAndMakeVisible(*slider);

        // Setup callback
        slider->onValueChanged = [](float value) {
            DBG("Slider value changed: " + juce::String(value));
        };
    }

    void paint (juce::Graphics& g) override {
        // Background color (same as plugin background)
        g.fillAll (juce::Colour (0xff1a1a1a));
    }

    void resized() override {
        auto bounds = getLocalBounds();

        // Position single slider in the center for now
        int sliderWidth = 50;
        int sliderHeight = 150;
        int x = (bounds.getWidth() - sliderWidth) / 2;
        int y = (bounds.getHeight() - sliderHeight) / 2;

        slider->setBounds(x, y, sliderWidth, sliderHeight);
    }

private:
    std::unique_ptr<VerticalBarSlider> slider;
};

