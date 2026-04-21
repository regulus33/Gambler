#pragma once

#include "../MidiMapping/MidiMapping.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Information about a mappable control
 */
struct MappableControlInfo {
    MidiControlId controlId = MidiControlId::COUNT;
    juce::Rectangle<int> bounds;  // Bounds in screen coordinates
    juce::String displayName;     // Human-readable name
    bool isActive = true;         // Whether control is currently visible/mappable
};
