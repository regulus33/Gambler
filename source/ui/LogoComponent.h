#pragma once

#include "BinaryData.h"
#include "Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace LogoConstants {
    // Overall component scale factor (applies to everything)
    constexpr float componentScale = 1.2f;

    // Vertical padding from the top of the parent component
    constexpr int topPadding = 5;

    // Scale factor for compact bottom row mode (logo shares space with step buttons)
    constexpr float compactBottomScale = 0.6f;

    // Fine-tune offsets for positioning within the allocated bounds (applied after centering)
    constexpr float offsetX = 0.0f;
    constexpr float offsetY = 0.0f;

    // Scale factor for the vectorized "Gambler" text path (tweak to adjust text size)
    constexpr float logoTextScale = 0.65f;

    // Padding between the text and the dice icon (to the right of text)
    constexpr float textToDicePadding = 8.0f;

    // Scale factor for the dice icon
    constexpr float diceScale = 1.2f;

    // Base dice size
    constexpr float baseDiceSize = 26.0f;
    constexpr float diceSize = baseDiceSize * diceScale;

    // Dice position offset (for fine-tuning vertical alignment with text)
    constexpr float diceYOffset = 2.0f;

    // Original dice SVG viewBox: 0 0 22.3 22.8
    constexpr float diceOriginalWidth = 22.3f;
    constexpr float diceOriginalHeight = 22.8f;

    // Original logo text SVG coordinate space: approx 163 x 68
    constexpr float logoTextOriginalWidth = 163.0f;
    constexpr float logoTextOriginalHeight = 68.0f;
}

/**
 * LogoComponent - Displays "Gambler" vectorized text with dice icon
 *
 * Layout: "Gambler" vector text path + textToDicePadding gap + dice icon
 * The component sizes itself to fit the content tightly.
 * Tweak logoTextScale and textToDicePadding in LogoConstants to adjust appearance.
 */
class LogoComponent : public juce::Component {
public:
    LogoComponent() {
        // Create the logo text and dice icon paths
        logoTextIcon = Icons::createLogoTextIconFull();
        diceIcon = Icons::createDiceIcon();

        // Calculate component size based on content
        calculateSize();
    }

    void paint(juce::Graphics& g) override {
        using namespace LogoConstants;

        g.setColour(juce::Colours::white);

        // Compute scaled content dimensions
        float es = effectiveScale();
        float scaledTextScale = logoTextScale * es;
        float scaledDiceSize = diceSize * es;
        float scaledPadding = textToDicePadding * es;

        float textH = logoTextOriginalHeight * scaledTextScale;
        float textW = logoTextOriginalWidth * scaledTextScale;
        float contentW = textW + scaledPadding + scaledDiceSize;
        float contentH = juce::jmax(textH, scaledDiceSize);

        // Centre content within component bounds, then apply offsets
        float startX = (static_cast<float>(getWidth()) - contentW) * 0.5f + offsetX;
        float startY = (static_cast<float>(getHeight()) - contentH) * 0.5f + offsetY;

        // Draw vectorized "Gambler" text
        float textY = startY + (contentH - textH) * 0.5f;
        juce::AffineTransform textTransform = juce::AffineTransform::scale(scaledTextScale)
            .translated(startX, textY);
        g.fillPath(logoTextIcon, textTransform);

        // Draw dice icon to the right of text
        float diceX = startX + textW + scaledPadding;
        float diceY = startY + (contentH - scaledDiceSize) * 0.5f + diceYOffset * es;

        float diceScaleFactor = scaledDiceSize / juce::jmax(diceOriginalWidth, diceOriginalHeight);
        juce::AffineTransform diceTransform = juce::AffineTransform::scale(diceScaleFactor)
            .translated(diceX, diceY);
        g.fillPath(diceIcon, diceTransform);
    }

    // Returns the calculated width of the component
    int getCalculatedWidth() const { return static_cast<int>(totalWidth); }

    // Returns the calculated height of the component
    int getCalculatedHeight() const { return static_cast<int>(totalHeight); }

    /**
     * Set an extra scale multiplier (e.g. for compact bottom row mode).
     * Defaults to 1.0 (no extra scaling).
     */
    void setModeScale(float scale) {
        if (modeScale_ != scale) {
            modeScale_ = scale;
            calculateSize();
            repaint();
        }
    }

private:
    float effectiveScale() const {
        return LogoConstants::componentScale * modeScale_;
    }

    void calculateSize() {
        using namespace LogoConstants;

        float es = effectiveScale();
        float scaledTextScale = logoTextScale * es;
        float scaledDiceSize = diceSize * es;
        float scaledPadding = textToDicePadding * es;

        float textW = logoTextOriginalWidth * scaledTextScale;
        float textH = logoTextOriginalHeight * scaledTextScale;
        totalWidth = textW + scaledPadding + scaledDiceSize;
        totalHeight = juce::jmax(textH, scaledDiceSize);
    }

    float modeScale_ = 1.0f;
    juce::Path logoTextIcon;
    juce::Path diceIcon;
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
};

