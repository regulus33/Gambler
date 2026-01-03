#pragma once

#include "SequencerControlButton.h"

/**
 * LengthMinusButton - Decreases sequence length by 1
 * 
 * Observer pattern implementation:
 * - mouseDown sets LENGTH_MINUS param to true
 * - mouseUp sets LENGTH_MINUS param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual decreaseLength() call
 */
class LengthMinusButton : public SequencerControlButton {
public:
    explicit LengthMinusButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::LENGTH_MINUS,
                                  MidiControlId::BUTTON_LENGTH_MINUS)
    {
        setIcon(Icons::createLengthMinusIcon());
        setLabel("Length");
    }
};

