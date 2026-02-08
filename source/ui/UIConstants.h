#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UIConstants {
    // Master Y offset for section titles (positive = down, negative = up)
    constexpr int titleYOffset = 10;

    // Centralized white color for step indicators (current step fill, active step border)
    // Adjust this to tweak the shade of white used throughout the UI
    static inline const juce::Colour stepIndicatorWhite = juce::Colours::white;
}

