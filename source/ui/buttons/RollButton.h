#pragma once

#include "SequencerControlButton.h"

/**
 * RollButton - Enables random mode while held (momentary)
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
