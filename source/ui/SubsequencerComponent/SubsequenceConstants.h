#pragma once

#include <juce_core/juce_core.h>

namespace SubsequenceControlsConstants {
    // Scaling factor to match SequenceGrid height (196px)
    // Current unscaled height: title(24) + gap(13) + bars(157) + gap(16) + buttons(15) = 225px
    // To fit in 196px with title fixed: (196 - 24) / (13 + 157 + 16 + 15) = 172 / 201 ≈ 0.856
    constexpr float heightScaleFactor = 1.1f;

    // Base dimensions (before scaling)
    constexpr int baseGapBelowTitle = 13;
    constexpr float baseBarWidth = 50.f;
    constexpr int baseBarHeight = 17;
    constexpr int baseBarSpacing = 3;
    constexpr int baseButtonHeight = 15;
    constexpr int baseGapAboveButtons = 16;

    // Scaled dimensions
    constexpr int gapBelowTitle = 3;
    constexpr float barWidth = baseBarWidth * heightScaleFactor;  // ~42.8px
    constexpr int barHeight = static_cast<int>(baseBarHeight * heightScaleFactor);  // ~14px
    constexpr int barSpacing = static_cast<int>(baseBarSpacing * heightScaleFactor);  // ~2px
    constexpr int buttonHeight = static_cast<int>(baseButtonHeight * heightScaleFactor);  // ~12px
    constexpr int gapAboveButtons = static_cast<int>(baseGapAboveButtons * heightScaleFactor);  // ~13px

    // Other dimensions (scaled)
    constexpr float barCornerRadius = 2.0f * heightScaleFactor;
    constexpr float borderWidth = 2.0f * heightScaleFactor;
    constexpr int numBars = 8;

    // Calculated bar stack dimensions
    constexpr int barStackHeight = numBars * barHeight + (numBars - 1) * barSpacing;  // ~134px

    // Buttons
    constexpr float buttonWidthRatio = 0.38f;
    constexpr float buttonWidth = barWidth * buttonWidthRatio;
    constexpr float buttonCornerRadius = 3.0f * heightScaleFactor;
    constexpr float buttonFontSize = 12.0f * heightScaleFactor;

    // Colors
    constexpr juce::uint32 defaultGreyHex = 0xff2a2a2a;  // Dark grey for inactive/out-of-length substeps
    constexpr juce::uint32 restGreyHex = 0xff505050;     // Lighter grey for rest substeps (matches StepButton)
    constexpr juce::uint32 buttonBgHex = 0xff484848;

    // Note label
    constexpr float noteLabelScale = 0.9f;
    constexpr float fontSize = 10.0f * noteLabelScale;
    constexpr float activeCircleDiameter = 14.0f;

    // Container
    constexpr int horizontalPadding = 10;  // Padding on each side
    constexpr int containerWidth = static_cast<int>(barWidth) + 2 * horizontalPadding;

    // Total height calculation
    constexpr int totalHeight = gapBelowTitle + barStackHeight + gapAboveButtons + buttonHeight;
}
