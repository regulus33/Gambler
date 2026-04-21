#pragma once

#include "SequenceGridConstants.h"
#include <juce_graphics/juce_graphics.h>

/**
 * SquareState - Represents the visual state of a single grid square
 */
struct SquareState {
    juce::Colour color { juce::Colour (SequenceGridConstants::defaultGreyHex) };
    bool hasBorder { false };      // Border indicates selected step for editing
    bool isCurrent { false };      // Current step (playback position) - white fill

    SquareState() = default;
    SquareState (juce::Colour c, bool border = false, bool current = false)
        : color (c), hasBorder (border), isCurrent (current) {}
};
