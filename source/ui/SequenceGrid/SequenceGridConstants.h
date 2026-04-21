#pragma once

#include <juce_graphics/juce_graphics.h>

namespace SequenceGridConstants {
    constexpr int gridRows = 8;
    constexpr int gridCols = 8;
    constexpr int squareSize = 21;  // 17 * 1.24 ≈ 21
    constexpr int squareSpacing = 4;  // 3 * 1.24 ≈ 3.72, rounded to 4
    constexpr float squareCornerRadius = 2.48f;  // 2.0 * 1.24
    constexpr float borderWidth = 2.48f;  // 2.0 * 1.24
    // Default colors
    constexpr juce::uint32 defaultGreyHex = 0xff808080;      // Grey for default squares
    constexpr juce::uint32 defaultDarkGreyHex = 0xff808080;  // Dark grey for rest steps
    constexpr juce::uint32 outOfBoundsGreyHex = 0xff3B3B3B;
    constexpr juce::uint32 dragPreviewOrangeHex = 0xFFFF8C00; // Orange color for drag preview
    // Window row opacity (for non-visible rows)
    constexpr float nonVisibleRowOpacity = 0.3f;  // Non-visible rows at 30% opacity
    constexpr float colourFadeSpeed = 8.0f;  // Lerp speed for colour transitions
    // Calculated dimensions
    constexpr int gridWidth = gridCols * squareSize + (gridCols - 1) * squareSpacing;   // 196px
    constexpr int gridHeight = gridRows * squareSize + (gridRows - 1) * squareSpacing;  // 196px
}
