#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UIConstants {
    // Configurable step indicator colours (defaults: white)
    // Active step = playback position fill colour
    // Selected step = editing selection outline colour
    inline juce::Colour activeStepColour { 0xFFFFF5E1 };          // light cream
    inline juce::Colour selectedStepOutlineColour { 0xFFFFF5E1 }; // light cream

    // Selected-step border fade (seconds)
    constexpr float borderFadeInDuration = 0.08f;    // Fast fade in
    constexpr float borderFadeOutDuration = 0.18f;   // Slightly slower fade out

    // Legacy alias kept for drag-highlight usage
    static inline const juce::Colour stepIndicatorWhite = juce::Colours::white;
}
