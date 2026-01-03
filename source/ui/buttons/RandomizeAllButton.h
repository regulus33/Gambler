#pragma once

#include "SequencerControlButton.h"

/**
 * RandomizeAllButton - Randomizes all steps in the sequence
 * 
 * Observer pattern implementation:
 * - mouseDown sets RANDOMIZE_ALL param to true
 * - mouseUp sets RANDOMIZE_ALL param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual randomize all action
 */
class RandomizeAllButton : public SequencerControlButton {
public:
    explicit RandomizeAllButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::RANDOMIZE_ALL,
                                  MidiControlId::BUTTON_RANDOMIZE_ALL)
    {
        setIcon(Icons::createRandomAllIcon());
        setLabel("Rand All");
    }
};

