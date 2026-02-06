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
 *
 * MIDI Mapping:
 * - Shares BUTTON_NUDGE_LEFT MIDI mapping (shift-aware routing in PluginProcessor)
 */
class RandomizeStepButton : public SequencerControlButton {
public:
    explicit RandomizeStepButton(PluginProcessor& processor)
        : SequencerControlButton(processor,
                                  Params::Keys::ID::RANDOMIZE_STEP,
                                  MidiControlId::BUTTON_NUDGE_LEFT)  // Shares mapping with NudgeLeft
    {
        setIcon(Icons::createDiceOneIcon());
        setLabel("Rand");
    }
};

