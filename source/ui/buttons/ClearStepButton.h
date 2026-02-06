#pragma once

#include "SequencerControlButton.h"

/**
 * ClearStepButton - Clears the currently selected step (sets to rest)
 *
 * Observer pattern implementation:
 * - mouseDown sets CLEAR_STEP param to true
 * - mouseUp sets CLEAR_STEP param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual clear step action
 *
 * MIDI Mapping:
 * - Shares BUTTON_LENGTH_PLUS MIDI mapping (shift-aware routing in PluginProcessor)
 */
class ClearStepButton : public SequencerControlButton {
public:
    explicit ClearStepButton(PluginProcessor& processor)
        : SequencerControlButton(processor,
                                  Params::Keys::ID::CLEAR_STEP,
                                  MidiControlId::BUTTON_LENGTH_PLUS)  // Shares mapping with LengthPlus
    {
        setIcon(Icons::createEraserIcon());
        setLabel("Erase");
    }
};

