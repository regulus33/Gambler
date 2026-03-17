#pragma once

#include "BinaryData.h"
#include "Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "../utility/ComponentSnapshotUtility.h"

namespace LogoConstants {
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

        // Draw vectorized "Gambler" text, scaled and vertically centered
        float textH = logoTextOriginalHeight * logoTextScale;
        float textW = logoTextOriginalWidth * logoTextScale;
        float textY = (getHeight() - textH) / 2.0f;

        juce::AffineTransform textTransform = juce::AffineTransform::scale(logoTextScale)
            .translated(0.0f, textY);
        g.fillPath(logoTextIcon, textTransform);

        // Draw dice icon to the right of text
        float diceX = textW + textToDicePadding;
        float diceY = (getHeight() - diceSize) / 2.0f + diceYOffset;

        float diceScaleFactor = diceSize / juce::jmax(diceOriginalWidth, diceOriginalHeight);
        juce::AffineTransform diceTransform = juce::AffineTransform::scale(diceScaleFactor)
            .translated(diceX, diceY);
        g.fillPath(diceIcon, diceTransform);
    }

    // Returns the calculated width of the component
    int getCalculatedWidth() const { return static_cast<int>(totalWidth); }

    // Returns the calculated height of the component
    int getCalculatedHeight() const { return static_cast<int>(totalHeight); }

private:
    void calculateSize() {
        using namespace LogoConstants;

        float textW = logoTextOriginalWidth * logoTextScale;
        float textH = logoTextOriginalHeight * logoTextScale;
        totalWidth = textW + textToDicePadding + diceSize;
        totalHeight = juce::jmax(textH, diceSize);
    }

    juce::Path logoTextIcon;
    juce::Path diceIcon;
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
};

