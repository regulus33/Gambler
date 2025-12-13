#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace NoteRangeControlsConstants {
    constexpr float borderWidth = 3.0f;
    constexpr float borderRadius = 3.0f;
}

/**
 * NoteRangeControlsComponent
 *
 * Panel displayed to the right of the sequencer steps/controls.
 * Will contain controls for note range settings.
 */
class NoteRangeControlsComponent : public juce::Component {
public:
    NoteRangeControlsComponent() = default;
    ~NoteRangeControlsComponent() override = default;

    void paint (juce::Graphics& g) override {
        using namespace NoteRangeControlsConstants;

        auto bounds = getLocalBounds().toFloat();

        // Draw white border inside the bounds
        g.setColour (juce::Colours::white);
        float inset = borderWidth * 0.5f;
        juce::Rectangle<float> borderBounds = bounds.reduced (inset);
        g.drawRoundedRectangle (borderBounds, borderRadius, borderWidth);
    }

    void resized() override {
        // Nothing to do yet - will layout child components here later
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteRangeControlsComponent)
};

