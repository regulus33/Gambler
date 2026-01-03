#pragma once

#include "SequencerControlButton.h"

/**
 * RollButton - Enables random mode while held (momentary)
 * 
 * Observer pattern implementation:
 * - mouseDown sets ROLL param to true
 * - mouseUp sets ROLL param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles setting isRandom state
 */
class RollButton : public SequencerControlButton {
public:
    explicit RollButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::ROLL,
                                  MidiControlId::BUTTON_ROLL)
    {
        setIcon(Icons::createDiceIcon());
        setLabel("Roll");
    }
};

