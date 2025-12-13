#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

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

namespace LogoIcons {
    /**
     * Creates the dice icon path
     * Original SVG viewBox: 0 0 22.3 22.8
     */
    inline juce::Path createDiceIcon() {
        juce::Path path;
        path.startNewSubPath(18.6f, 0.0f);
        path.lineTo(3.7f, 0.0f);
        path.cubicTo(2.7f, 0.0f, 1.8f, 0.4f, 1.1f, 1.1f);
        path.cubicTo(0.4f, 1.8f, 0.0f, 2.8f, 0.0f, 3.8f);
        path.lineTo(0.0f, 19.0f);
        path.cubicTo(0.0f, 20.0f, 0.4f, 20.9f, 1.1f, 21.6f);
        path.cubicTo(1.8f, 22.4f, 2.7f, 22.8f, 3.7f, 22.8f);
        path.lineTo(18.6f, 22.8f);
        path.cubicTo(19.5f, 22.8f, 20.5f, 22.4f, 21.2f, 21.6f);
        path.cubicTo(21.9f, 20.9f, 22.3f, 20.0f, 22.3f, 19.0f);
        path.lineTo(22.3f, 3.8f);
        path.cubicTo(22.3f, 2.8f, 21.9f, 1.8f, 21.2f, 1.1f);
        path.cubicTo(20.5f, 0.4f, 19.5f, 0.0f, 18.6f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath(20.4f, 19.0f);
        path.cubicTo(20.4f, 19.5f, 20.2f, 19.9f, 19.9f, 20.3f);
        path.cubicTo(19.5f, 20.7f, 19.0f, 20.9f, 18.6f, 20.9f);
        path.lineTo(3.7f, 20.9f);
        path.cubicTo(3.2f, 20.9f, 2.7f, 20.7f, 2.4f, 20.3f);
        path.cubicTo(2.1f, 19.9f, 1.9f, 19.5f, 1.9f, 19.0f);
        path.lineTo(1.9f, 3.8f);
        path.cubicTo(1.9f, 3.3f, 2.1f, 2.8f, 2.4f, 2.5f);
        path.cubicTo(2.7f, 2.1f, 3.2f, 1.9f, 3.7f, 1.9f);
        path.lineTo(18.6f, 1.9f);
        path.cubicTo(19.0f, 1.9f, 19.5f, 2.1f, 19.9f, 2.5f);
        path.cubicTo(20.2f, 2.8f, 20.4f, 3.3f, 20.4f, 3.8f);
        path.lineTo(20.4f, 19.0f);
        path.closeSubPath();
        path.startNewSubPath(8.3f, 7.1f);
        path.cubicTo(8.3f, 7.4f, 8.3f, 7.7f, 8.1f, 7.9f);
        path.cubicTo(8.0f, 8.1f, 7.7f, 8.3f, 7.5f, 8.4f);
        path.cubicTo(7.2f, 8.5f, 7.0f, 8.6f, 6.7f, 8.5f);
        path.cubicTo(6.4f, 8.4f, 6.2f, 8.3f, 6.0f, 8.1f);
        path.cubicTo(5.8f, 7.9f, 5.6f, 7.7f, 5.6f, 7.4f);
        path.cubicTo(5.5f, 7.1f, 5.6f, 6.8f, 5.7f, 6.6f);
        path.cubicTo(5.8f, 6.3f, 6.0f, 6.1f, 6.2f, 5.9f);
        path.cubicTo(6.4f, 5.8f, 6.7f, 5.7f, 7.0f, 5.7f);
        path.cubicTo(7.3f, 5.7f, 7.7f, 5.8f, 7.9f, 6.1f);
        path.cubicTo(8.2f, 6.4f, 8.3f, 6.7f, 8.3f, 7.1f);
        path.closeSubPath();
        path.startNewSubPath(16.7f, 7.1f);
        path.cubicTo(16.7f, 7.4f, 16.6f, 7.7f, 16.5f, 7.9f);
        path.cubicTo(16.3f, 8.1f, 16.1f, 8.3f, 15.8f, 8.4f);
        path.cubicTo(15.6f, 8.5f, 15.3f, 8.6f, 15.0f, 8.5f);
        path.cubicTo(14.8f, 8.4f, 14.5f, 8.3f, 14.3f, 8.1f);
        path.cubicTo(14.1f, 7.9f, 14.0f, 7.7f, 13.9f, 7.4f);
        path.cubicTo(13.9f, 7.1f, 13.9f, 6.8f, 14.0f, 6.6f);
        path.cubicTo(14.1f, 6.3f, 14.3f, 6.1f, 14.5f, 5.9f);
        path.cubicTo(14.8f, 5.8f, 15.0f, 5.7f, 15.3f, 5.7f);
        path.cubicTo(15.7f, 5.7f, 16.0f, 5.8f, 16.3f, 6.1f);
        path.cubicTo(16.6f, 6.4f, 16.7f, 6.7f, 16.7f, 7.1f);
        path.closeSubPath();
        path.startNewSubPath(8.3f, 15.6f);
        path.cubicTo(8.3f, 15.9f, 8.3f, 16.2f, 8.1f, 16.4f);
        path.cubicTo(8.0f, 16.7f, 7.7f, 16.8f, 7.5f, 17.0f);
        path.cubicTo(7.2f, 17.1f, 7.0f, 17.1f, 6.7f, 17.0f);
        path.cubicTo(6.4f, 17.0f, 6.2f, 16.8f, 6.0f, 16.6f);
        path.cubicTo(5.8f, 16.4f, 5.6f, 16.2f, 5.6f, 15.9f);
        path.cubicTo(5.5f, 15.6f, 5.6f, 15.4f, 5.7f, 15.1f);
        path.cubicTo(5.8f, 14.8f, 6.0f, 14.6f, 6.2f, 14.5f);
        path.cubicTo(6.4f, 14.3f, 6.7f, 14.2f, 7.0f, 14.2f);
        path.cubicTo(7.3f, 14.2f, 7.7f, 14.4f, 7.9f, 14.6f);
        path.cubicTo(8.2f, 14.9f, 8.3f, 15.3f, 8.3f, 15.6f);
        path.closeSubPath();
        path.startNewSubPath(12.5f, 11.4f);
        path.cubicTo(12.5f, 11.7f, 12.4f, 11.9f, 12.3f, 12.2f);
        path.cubicTo(12.1f, 12.4f, 11.9f, 12.6f, 11.7f, 12.7f);
        path.cubicTo(11.4f, 12.8f, 11.1f, 12.8f, 10.9f, 12.8f);
        path.cubicTo(10.6f, 12.7f, 10.3f, 12.6f, 10.1f, 12.4f);
        path.cubicTo(10.0f, 12.2f, 9.8f, 11.9f, 9.8f, 11.7f);
        path.cubicTo(9.7f, 11.4f, 9.7f, 11.1f, 9.8f, 10.8f);
        path.cubicTo(10.0f, 10.6f, 10.1f, 10.3f, 10.4f, 10.2f);
        path.cubicTo(10.6f, 10.0f, 10.9f, 10.0f, 11.1f, 10.0f);
        path.cubicTo(11.5f, 10.0f, 11.9f, 10.1f, 12.1f, 10.4f);
        path.cubicTo(12.4f, 10.6f, 12.5f, 11.0f, 12.5f, 11.4f);
        path.closeSubPath();
        path.startNewSubPath(16.7f, 15.6f);
        path.cubicTo(16.7f, 15.9f, 16.6f, 16.2f, 16.5f, 16.4f);
        path.cubicTo(16.3f, 16.7f, 16.1f, 16.8f, 15.8f, 17.0f);
        path.cubicTo(15.6f, 17.1f, 15.3f, 17.1f, 15.0f, 17.0f);
        path.cubicTo(14.8f, 17.0f, 14.5f, 16.8f, 14.3f, 16.6f);
        path.cubicTo(14.1f, 16.4f, 14.0f, 16.2f, 13.9f, 15.9f);
        path.cubicTo(13.9f, 15.6f, 13.9f, 15.4f, 14.0f, 15.1f);
        path.cubicTo(14.1f, 14.8f, 14.3f, 14.6f, 14.5f, 14.5f);
        path.cubicTo(14.8f, 14.3f, 15.0f, 14.2f, 15.3f, 14.2f);
        path.cubicTo(15.7f, 14.2f, 16.0f, 14.4f, 16.3f, 14.6f);
        path.cubicTo(16.6f, 14.9f, 16.7f, 15.3f, 16.7f, 15.6f);
        path.closeSubPath();
        return path;
    }
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
        diceIcon = LogoIcons::createDiceIcon();

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

    juce::Typeface::Ptr interBoldFont;
    juce::Path diceIcon;
    float textWidth = 0.0f;
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
};

