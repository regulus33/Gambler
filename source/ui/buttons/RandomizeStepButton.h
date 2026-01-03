#pragma once

#include "SequencerControlButton.h"

/**
 * RandomizeStepButton - Randomizes the currently selected step's note
 * 
 * Observer pattern implementation:
 * - mouseDown sets RANDOMIZE_STEP param to true
 * - mouseUp sets RANDOMIZE_STEP param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual randomize step action
 */
class RandomizeStepButton : public SequencerControlButton {
public:
    explicit RandomizeStepButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::RANDOMIZE_STEP,
                                  MidiControlId::BUTTON_RANDOMIZE_STEP)
    {
        setIcon(Icons::createDiceOneIcon());
        setLabel("Rand");
    }
};

