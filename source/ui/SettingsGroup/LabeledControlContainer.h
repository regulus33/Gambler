#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * LabeledControlContainer
 *
 * A simple container that lays out a label and control horizontally.
 */
class LabeledControlContainer : public juce::Component {
public:
    LabeledControlContainer(juce::Component& label, juce::Component& control, int labelWidth)
        : label_(label), control_(control), labelWidth_(labelWidth) {
        addAndMakeVisible(label_);
        addAndMakeVisible(control_);
    }

    void resized() override {
        auto area = getLocalBounds();
        label_.setBounds(area.removeFromLeft(labelWidth_));
        control_.setBounds(area);
    }

private:
    juce::Component& label_;
    juce::Component& control_;
    int labelWidth_;
};
