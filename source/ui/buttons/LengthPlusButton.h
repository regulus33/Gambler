#pragma once

#include "SequencerControlButton.h"

/**
 * LengthPlusButton - Increases sequence length by 1
 * 
 * Observer pattern implementation:
 * - mouseDown sets LENGTH_PLUS param to true
 * - mouseUp sets LENGTH_PLUS param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual increaseLength() call
 */
class LengthPlusButton : public SequencerControlButton {
public:
    explicit LengthPlusButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::LENGTH_PLUS,
                                  MidiControlId::BUTTON_LENGTH_PLUS)
    {
        setIcon(Icons::createLengthPlusIcon());
        setLabel("Length");
    }
};

