#pragma once

#include "SequencerControlButton.h"

/**
 * ClearWindowButton - Clears all notes in the current 8-step view window
 * 
 * Observer pattern implementation:
 * - mouseDown sets CLEAR_WINDOW param to true
 * - mouseUp sets CLEAR_WINDOW param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual clear window action
 */
class ClearWindowButton : public SequencerControlButton {
public:
    explicit ClearWindowButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::CLEAR_WINDOW,
                                  MidiControlId::BUTTON_CLEAR_WINDOW)
    {
        setIcon(Icons::createTrashIcon());
        setLabel("Clear");
    }
};

