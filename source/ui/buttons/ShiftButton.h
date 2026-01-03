#pragma once

#include "SequencerControlButton.h"

/**
 * ShiftButton - Toggles visibility of secondary button row
 * 
 * Observer pattern implementation:
 * - mouseDown sets SHIFT param to true
 * - mouseUp sets SHIFT param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles toggling shift state
 * 
 * Note: This is a toggle button - the param stays true until clicked again
 */
class ShiftButton : public SequencerControlButton {
public:
    explicit ShiftButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::SHIFT,
                                  MidiControlId::BUTTON_SHIFT)
    {
        setIcon(Icons::createShiftIcon());
        setLabel("Shift");
    }
};

