#pragma once

#include "SequencerControlButton.h"

/**
 * PageRightButton - Pages the view window right by 8 steps
 * 
 * Observer pattern implementation:
 * - mouseDown sets INDEX_RIGHT param to true
 * - mouseUp sets INDEX_RIGHT param to false
 * - Visual state reflects param value (white when true, grey when false)
 * - PluginProcessor::parameterChanged handles the actual page right action
 */
class PageRightButton : public SequencerControlButton {
public:
    explicit PageRightButton(PluginProcessor& processor)
        : SequencerControlButton(processor, 
                                  Params::Keys::ID::INDEX_RIGHT,
                                  MidiControlId::BUTTON_PAGE_RIGHT)
    {
        setIcon(Icons::createIndexRightIcon());
        setLabel("Page");
    }
};

