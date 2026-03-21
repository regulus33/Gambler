#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UIConstants {
    // Configurable step indicator colours (defaults: white)
    // Active step = playback position fill colour
    // Selected step = editing selection outline colour
    inline juce::Colour activeStepColour { 0xFFFFF5E1 };          // light cream
    inline juce::Colour selectedStepOutlineColour { 0xFFFFF5E1 }; // light cream

    // Legacy alias kept for drag-highlight usage
    static inline const juce::Colour stepIndicatorWhite = juce::Colours::white;
}
