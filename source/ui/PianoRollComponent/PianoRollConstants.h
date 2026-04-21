#pragma once

#include <juce_graphics/juce_graphics.h>

namespace PianoRollConstants {
    // Component size
    constexpr int componentWidth = 422;
    constexpr int componentHeight = 206;
    // Base (unscaled) font and active circle — scaled dynamically to match component size
    constexpr float baseFontSize = 12.0f;
    constexpr float baseActiveCircleRadius = 13.0f;
    // Scale factor for note text on keys (tweak to resize labels)
    constexpr float noteTextScale = 1.2f;
    // Key border
    constexpr float keyBorderWidth = 1.0f;
    // Black key positioning (in unscaled path coordinates)
    constexpr float blackKeyWidth = 40.0f;
    constexpr float blackKeyShift = 30.0f;
    // Unselected key colors
    const juce::Colour unselectedWhiteKeyColor = juce::Colours::white;
    const juce::Colour unselectedBlackKeyColor = juce::Colour (186, 186, 186);
    // Colour fade animation
    constexpr float colourFadeSpeed = 10.0f;
    // Active note circle fade (seconds)
    constexpr float activeCircleFadeInDuration = 0.06f;
    constexpr float activeCircleFadeOutDuration = 0.15f;
}
