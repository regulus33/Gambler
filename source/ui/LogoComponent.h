#pragma once

#include "BinaryData.h"
#include "Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include "../utility/ComponentSnapshotUtility.h"

namespace LogoConstants {
    // Scale factor to tweak overall logo size (1.0 = base size)
    constexpr float scaleFactor = 1.2f;

    // Base sizes (before scaling)
    constexpr float baseFontSize = 40.0f;
    constexpr float baseDiceSize = 26.0f;
    constexpr float baseGap = 5.0f;

    // Scaled sizes (used in rendering)
    constexpr float fontSize = baseFontSize * scaleFactor;
    constexpr float diceSize = baseDiceSize * scaleFactor;
    constexpr float gapBetweenTextAndDice = baseGap * scaleFactor;

    // Extra padding to prevent text clipping (font metrics don't always account for full glyph width)
    constexpr float textPadding = 2.0f;

    // Dice position offset (for fine-tuning vertical alignment with text)
    constexpr float diceYOffset = 2.0f;

    // Original dice SVG viewBox: 0 0 22.3 22.8
    constexpr float diceOriginalWidth = 22.3f;
    constexpr float diceOriginalHeight = 22.8f;
}

/**
 * LogoComponent - Displays "Gambler" text with dice icon
 *
 * Layout: "Gambler" text (Inter bold 40px white) + 5px gap + 26x26 dice icon
 * The component sizes itself to fit the content tightly.
 */
class LogoComponent : public juce::Component {
public:
    LogoComponent() {
        // Load Inter 28pt Bold static font (designed for larger display sizes)
        interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_28ptBold_ttf,
            BinaryData::Inter_28ptBold_ttfSize);

        // Create the dice icon path
        diceIcon = Icons::createDiceIcon();

        // Calculate component size based on content
        calculateSize();
    }

    void paint(juce::Graphics& g) override {
        using namespace LogoConstants;

        // Draw "Gambler" text
        g.setColour(juce::Colours::white);
        juce::Font font(interBoldFont);
        font.setHeight(fontSize);
        g.setFont(font);

        // Text is drawn at left edge, vertically centered
        float textY = (getHeight() - fontSize) / 2.0f;
        g.drawText("Gambler", 0, static_cast<int>(textY),
                   static_cast<int>(textWidth), static_cast<int>(fontSize),
                   juce::Justification::centredLeft, false);

        // Draw dice icon to the right of text
        float diceX = textWidth + gapBetweenTextAndDice;
        float diceY = (getHeight() - diceSize) / 2.0f + diceYOffset;

        // Scale the dice icon to target size
        float scale = diceSize / juce::jmax(diceOriginalWidth, diceOriginalHeight);

        juce::AffineTransform transform = juce::AffineTransform::scale(scale)
            .translated(diceX, diceY);

        g.fillPath(diceIcon, transform);

        // Take snapshot after first paint when component is fully rendered
        if (!snapshotTaken) {
            snapshotTaken = true;
            takeSnapshot();
        }
    }

    // Returns the calculated width of the component
    int getCalculatedWidth() const { return static_cast<int>(totalWidth); }

    // Returns the calculated height of the component
    int getCalculatedHeight() const { return static_cast<int>(totalHeight); }

private:
    void calculateSize() {
        using namespace LogoConstants;

        // Create font to measure text width
        juce::Font font(interBoldFont);
        font.setHeight(fontSize);

        textWidth = font.getStringWidthFloat("Gambler") + textPadding;
        totalWidth = textWidth + gapBetweenTextAndDice + diceSize;
        totalHeight = juce::jmax(fontSize, diceSize);
    }

    void takeSnapshot() {
        #if JUCE_DEBUG
        // Take a high-resolution snapshot (2x scale for crisp output)
        ComponentSnapshotUtility::saveSnapshot(
            *this,
            "/Users/you/Workspace/learning/Gambler",
            "LogoComponent",
            2.0f
        );
        #endif
    }

    juce::Typeface::Ptr interBoldFont;
    juce::Path diceIcon;
    float textWidth = 0.0f;
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    bool snapshotTaken = false;
};