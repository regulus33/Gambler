#pragma once

#include "SequencerControlButton.h"

/**
 * ShuffleButton - Randomly shuffles the order of all steps
 * 
 * Observer pattern implementation:
 * - mouseDown sets SHUFFLE param to true
 * - mouseUp sets SHUFFLE param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual shuffle action
 */
class ShuffleButton : public SequencerControlButton {
public:
    explicit ShuffleButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::SHUFFLE,
                                  MidiControlId::BUTTON_SHUFFLE)
    {
        setIcon(Icons::createShuffleIcon());
        setLabel("Shuffle");
    }
};

