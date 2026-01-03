#pragma once

#include "SequencerControlButton.h"

/**
 * NudgeRightButton - Shifts all notes right by 1 step
 * 
 * Observer pattern implementation:
 * - mouseDown sets NUDGE_RIGHT param to true
 * - mouseUp sets NUDGE_RIGHT param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual nudgeRight() call
 */
class NudgeRightButton : public SequencerControlButton {
public:
    explicit NudgeRightButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::NUDGE_RIGHT,
                                  MidiControlId::BUTTON_NUDGE_RIGHT)
    {
        setIcon(Icons::createNudgeRightIcon());
        setLabel("Nudge");
    }
};

