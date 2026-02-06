#pragma once

#include "SequencerControlButton.h"

/**
 * DuplicateButton - Doubles sequence length and copies content
 *
 * Observer pattern implementation:
 * - mouseDown sets DUPLICATE param to true
 * - mouseUp sets DUPLICATE param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual duplicate action
 *
 * MIDI Mapping:
 * - Shares BUTTON_PAGE_LEFT MIDI mapping (shift-aware routing in PluginProcessor)
 */
class DuplicateButton : public SequencerControlButton {
public:
    explicit DuplicateButton(PluginProcessor& processor)
        : SequencerControlButton(processor,
                                  Params::Keys::ID::DUPLICATE,
                                  MidiControlId::BUTTON_PAGE_LEFT)  // Shares mapping with PageLeft
    {
        setIcon(Icons::createDupIcon());
        setLabel("Dup");
    }
};

