#pragma once

#include "SequencerControlButton.h"

/**
 * PageLeftButton - Pages the view window left by 8 steps
 * 
 * Observer pattern implementation:
 * - mouseDown sets INDEX_LEFT param to true
 * - mouseUp sets INDEX_LEFT param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual page left action
 */
class PageLeftButton : public SequencerControlButton {
public:
    explicit PageLeftButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::INDEX_LEFT,
                                  MidiControlId::BUTTON_PAGE_LEFT)
    {
        setIcon(Icons::createIndexLeftIcon());
        setLabel("Page");
    }
};

