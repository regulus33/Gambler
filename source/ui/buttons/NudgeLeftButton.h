#pragma once

#include "SequencerControlButton.h"

/**
 * NudgeLeftButton - Shifts all notes left by 1 step
 * 
 * Observer pattern implementation:
 * - mouseDown sets NUDGE_LEFT param to true
 * - mouseUp sets NUDGE_LEFT param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual nudgeLeft() call
 */
class NudgeLeftButton : public SequencerControlButton {
public:
    explicit NudgeLeftButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::NUDGE_LEFT,
                                  MidiControlId::BUTTON_NUDGE_LEFT)
    {
        setIcon(Icons::createNudgeLeftIcon());
        setLabel("Nudge");
    }
};

