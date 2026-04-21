#pragma once

#include "../ToggleIconButton.h"
#include "../sliders/SubdivisionSlider.h"
#include <juce_graphics/juce_graphics.h>

namespace NavBarConstants {
    constexpr int height = 45;
    constexpr int marginLeft = 8;
    constexpr int marginRight = 35;

    // Per-component scale factors — tweak individually to resize each control
    constexpr float compactTopScale    = 0.8f;
    constexpr float compactBottomScale = 0.8f;
    constexpr float subdivisionScale   = 0.8f;
    constexpr float exportScale        = 0.8f;
    constexpr float scaleLatchScale    = 0.8f;
    constexpr float playPauseScale     = 0.8f;
    constexpr float stopScale          = 0.8f;
    constexpr float midiLearnScale     = 0.8f;
    constexpr float bakeScaleScale     = 0.8f;
    constexpr float bakeRangeScale     = 0.8f;
    constexpr float settingsScale      = 0.8f;

    // Base sizes (before scaling)
    constexpr float baseButtonSize     = static_cast<float>(ToggleIconButtonConstants::totalSize);
    constexpr float baseSubdivSize     = static_cast<float>(SubdivisionSliderConstants::totalSize);

    // Spacing
    constexpr int buttonGap = 6;

    // Settings icon scale factor
    constexpr float settingsIconScale = 1.0f;

    // Transport button colors
    const juce::Colour inactiveColor = juce::Colours::white;
    const juce::Colour activePlayColor = juce::Colour(0xff4ade80);
    const juce::Colour activeStopColor = juce::Colour(0xfff87171);

    // Compact mode pulse glow color (gold)
    const juce::Colour compactPulseColor = juce::Colour(0xFFFFD700);
}
