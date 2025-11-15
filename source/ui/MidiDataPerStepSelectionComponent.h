#pragma once

#include "VerticalBarSlider.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MidiDataPerStepSelectionComponent
 *
 * Displays MIDI CC parameter selection for each visible step.
 * Size: 408 x 199 pixels (matches PianoRollComponent)
 *
 * Layout using juce::Grid:
 * - Row 0 (2/10 height): 2 columns (9/10 width empty space, 1/10 width button)
 * - Row 1 (2/10 height): 7 columns (dummy components for dropdowns)
 * - Row 2 (6/10 height): 7 columns (vertical bar sliders)
 * - 4px spacing between all grid items
 */
class MidiDataPerStepSelectionComponent : public juce::Component {
public:
    MidiDataPerStepSelectionComponent() {
        // Row 0: Empty space + dummy button
        spacer = std::make_unique<juce::Component>();
        addAndMakeVisible(*spacer);

        dummyButton = std::make_unique<DummyButton>();
        addAndMakeVisible(*dummyButton);

        // Row 1: 7 dummy components (placeholders for dropdowns)
        for (int i = 0; i < 7; ++i) {
            auto dummy = std::make_unique<DummyComponent>();
            dummyDropdowns.push_back(std::move(dummy));
            addAndMakeVisible(*dummyDropdowns.back());
        }

        // Row 2: 7 vertical bar sliders
        for (int i = 0; i < 7; ++i) {
            auto slider = std::make_unique<VerticalBarSlider>();
            sliders.push_back(std::move(slider));
            addAndMakeVisible(*sliders.back());

            sliders.back()->onValueChanged = [i](float value) {
                DBG("Slider " + juce::String(i) + " value: " + juce::String(value));
            };
        }
    }

    void paint (juce::Graphics& g) override {
        // Background color (same as plugin background)
        g.fillAll (juce::Colour (0xff1a1a1a));
    }

    void resized() override {
        using Track = juce::Grid::TrackInfo;
        using Fr = juce::Grid::Fr;

        juce::Grid grid;

        // Define rows: 2/10, 2/10, 6/10 of total height
        grid.templateRows = {
            Track(Fr(2)),  // Row 0: 2/10 height
            Track(Fr(2)),  // Row 1: 2/10 height
            Track(Fr(6))   // Row 2: 6/10 height
        };

        // Define 7 equal columns (for the 7 steps)
        grid.templateColumns = {
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1)),
            Track(Fr(1))
        };

        // Set spacing
        grid.columnGap = juce::Grid::Px(4);
        grid.rowGap = juce::Grid::Px(4);

        // Add items to grid
        // Row 0: Spacer spans first 6 columns (9/10 width), Button in last column (1/10 width)
        grid.items.add(juce::GridItem(*spacer).withArea(1, 1, 2, 7));      // Row 1, Col 1-6 (span 6 cols)
        grid.items.add(juce::GridItem(*dummyButton).withArea(1, 7));       // Row 1, Col 7

        // Row 1: 7 dummy dropdowns
        for (int i = 0; i < 7; ++i) {
            grid.items.add(juce::GridItem(*dummyDropdowns[i]).withArea(2, i + 1));  // Row 2, Col 1-7
        }

        // Row 2: 7 sliders
        for (int i = 0; i < 7; ++i) {
            grid.items.add(juce::GridItem(*sliders[i]).withArea(3, i + 1));  // Row 3, Col 1-7
        }

        grid.performLayout(getLocalBounds());
    }

private:
    /**
     * DummyComponent - Placeholder component for testing grid layout
     */
    class DummyComponent : public juce::Component {
    public:
        void paint(juce::Graphics& g) override {
            g.setColour(juce::Colour(0xff484848));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);

            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 3.0f, 2.0f);
        }
    };

    /**
     * DummyButton - Placeholder button for testing grid layout
     */
    class DummyButton : public juce::Component {
    public:
        void paint(juce::Graphics& g) override {
            g.setColour(juce::Colour(0xff484848));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);

            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 3.0f, 2.0f);
            g.drawText("Button", getLocalBounds(), juce::Justification::centred);
        }
    };

    std::unique_ptr<juce::Component> spacer;
    std::unique_ptr<DummyButton> dummyButton;
    std::vector<std::unique_ptr<DummyComponent>> dummyDropdowns;
    std::vector<std::unique_ptr<VerticalBarSlider>> sliders;
};

