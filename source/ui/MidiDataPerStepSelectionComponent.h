#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MidiDataPerStepSelectionComponent
 * 
 * Placeholder component for MIDI data per step selection.
 * This will eventually allow users to select which MIDI CC parameter to edit per step.
 * 
 * Size: 408 x 199 pixels (matches PianoRollComponent)
 */
class MidiDataPerStepSelectionComponent : public juce::Component {
public:
    MidiDataPerStepSelectionComponent() = default;
    
    void paint (juce::Graphics& g) override {
        // Placeholder: grey background
        g.fillAll (juce::Colour (0xff3a3a3a));
    }
    
    void resized() override {
        // TODO: Layout MIDI CC selection controls
    }
};

