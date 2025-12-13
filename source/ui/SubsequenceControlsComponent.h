#pragma once

#include "BinaryData.h"
#include "SequenceGrid.h"  // For SquareState and constants
#include "UIConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <array>

namespace SubsequenceControlsConstants {
    // Title
    constexpr float titleFontSize = 16.0f;
    constexpr int titleHeight = 24;
    constexpr int gapBelowTitle = 13;  // Same as SequenceOverviewComponent
    // Bar dimensions (2.5× SequenceGrid square width)
    constexpr float barWidth = 42.5f;
    constexpr int barHeight = 17;  // Same as SequenceGrid square height
    constexpr int barSpacing = 3;  // Same as SequenceGrid spacing
    constexpr float barCornerRadius = 2.0f;  // Same as SequenceGrid
    constexpr float borderWidth = 2.0f;
    constexpr int numBars = 8;
    // Calculated bar stack dimensions
    constexpr int barStackHeight = numBars * barHeight + (numBars - 1) * barSpacing;  // 157px
    // Buttons
    constexpr float buttonWidthRatio = 0.38f;
    constexpr float buttonWidth = barWidth * buttonWidthRatio;  // ~16.15px
    constexpr int buttonHeight = 15;
    constexpr int gapAboveButtons = 16;
    constexpr float buttonCornerRadius = 3.0f;
    constexpr float buttonFontSize = 12.0f;
    // Colors
    constexpr juce::uint32 defaultGreyHex = 0xff484848;  // Same as button background
    constexpr juce::uint32 buttonBgHex = 0xff484848;
    // Container
    constexpr int horizontalPadding = 10;  // Padding on each side
    constexpr int containerWidth = static_cast<int>(barWidth) + 2 * horizontalPadding;  // ~62px
}

/**
 * BarState - Represents the visual state of a single bar
 */
struct BarState {
    juce::Colour color { juce::Colour (SubsequenceControlsConstants::defaultGreyHex) };
    bool hasBorder { false };

    BarState() = default;
    BarState (juce::Colour c, bool border = false) : color (c), hasBorder (border) {}
};

/**
 * SubsequenceBar - A vertical stack of 8 clickable rectangles
 */
class SubsequenceBar : public juce::Component {
public:
    SubsequenceBar() {
        // Initialize all bars to default grey
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            barStates[i] = BarState();
        }
    }

    void paint (juce::Graphics& g) override {
        using namespace SubsequenceControlsConstants;

        for (int i = 0; i < numBars; ++i) {
            const auto& state = barStates[i];

            float y = static_cast<float>(i * (barHeight + barSpacing));

            auto barBounds = juce::Rectangle<float> (0.0f, y, barWidth, static_cast<float>(barHeight));

            // Fill bar with color
            g.setColour (state.color);
            g.fillRoundedRectangle (barBounds, barCornerRadius);

            // Draw white border if enabled
            if (state.hasBorder) {
                g.setColour (juce::Colours::white);
                g.drawRoundedRectangle (barBounds.reduced (0.5f), barCornerRadius, borderWidth);
            }
        }
    }

    void mouseDown (const juce::MouseEvent& event) override {
        using namespace SubsequenceControlsConstants;

        // Determine which bar was clicked
        int clickedIndex = -1;
        float y = static_cast<float>(event.y);

        for (int i = 0; i < numBars; ++i) {
            float barY = static_cast<float>(i * (barHeight + barSpacing));
            float barBottom = barY + static_cast<float>(barHeight);

            if (y >= barY && y < barBottom) {
                clickedIndex = i;
                break;
            }
        }

        if (clickedIndex >= 0 && onBarClicked) {
            onBarClicked (clickedIndex);
        }
    }

    /**
     * Set the state of a single bar
     * @param index Bar index (0-7)
     * @param state The new state for the bar
     */
    void setBarState (int index, const BarState& state) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            barStates[index] = state;
            repaint();
        }
    }

    /**
     * Set the color of a single bar (without changing border state)
     * @param index Bar index (0-7)
     * @param color The new color
     */
    void setBarColor (int index, juce::Colour color) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            barStates[index].color = color;
            repaint();
        }
    }

    /**
     * Set the border visibility of a single bar (without changing color)
     * @param index Bar index (0-7)
     * @param hasBorder Whether to show the border
     */
    void setBarBorder (int index, bool hasBorder) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            barStates[index].hasBorder = hasBorder;
            repaint();
        }
    }

    /**
     * Set all bar states at once (for efficient batch updates)
     * @param states Array of 8 BarState
     */
    void setAllBarStates (const std::array<BarState, SubsequenceControlsConstants::numBars>& states) {
        barStates = states;
        repaint();
    }

    /**
     * Clear all borders
     */
    void clearAllBorders() {
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            barStates[i].hasBorder = false;
        }
        repaint();
    }

    /**
     * Get the state of a single bar
     */
    const BarState& getBarState (int index) const {
        static BarState defaultState;
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return barStates[index];
        }
        return defaultState;
    }

    // Callback for bar clicks
    std::function<void(int)> onBarClicked;

private:
    std::array<BarState, SubsequenceControlsConstants::numBars> barStates;
};

/**
 * SubsequenceButton - Simple button for +/- actions
 */
class SubsequenceButton : public juce::Component {
public:
    explicit SubsequenceButton (const juce::String& labelText)
        : label (labelText)
    {
        interTypeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
    }

    void paint (juce::Graphics& g) override {
        using namespace SubsequenceControlsConstants;
        auto bounds = getLocalBounds().toFloat();

        // Background
        g.setColour (juce::Colour (buttonBgHex));
        g.fillRoundedRectangle (bounds, buttonCornerRadius);

        // Text
        g.setColour (juce::Colours::white);
        juce::Font font (juce::FontOptions (interTypeface).withHeight (buttonFontSize));
        font.setBold (true);
        g.setFont (font);
        g.drawText (label, bounds.toNearestInt(), juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent& /*event*/) override {
        if (onClick)
            onClick();
    }

    std::function<void()> onClick;

private:
    juce::String label;
    juce::Typeface::Ptr interTypeface;
};

/**
 * SubsequenceControlsComponent
 *
 * Container for the Subsequence controls UI.
 * - Title "Sub" at the top
 * - SubsequenceBar (8 vertical clickable rectangles)
 * - Two buttons: "-" and "+"
 */
class SubsequenceControlsComponent : public juce::Component {
public:
    SubsequenceControlsComponent()
        : interBoldFont (juce::Typeface::createSystemTypefaceFor (
              BinaryData::Inter_24ptBold_ttf,
              BinaryData::Inter_24ptBold_ttfSize)),
          minusButton ("-"),
          plusButton ("+")
    {
        addAndMakeVisible (subsequenceBar);
        addAndMakeVisible (minusButton);
        addAndMakeVisible (plusButton);

        // Setup bar click callback
        subsequenceBar.onBarClicked = [this] (int index) {
            if (onBarClicked)
                onBarClicked (index);
        };

        // Setup button callbacks
        minusButton.onClick = [this]() {
            if (onMinusClicked)
                onMinusClicked();
        };

        plusButton.onClick = [this]() {
            if (onPlusClicked)
                onPlusClicked();
        };
    }

    void paint (juce::Graphics& g) override {
        using namespace SubsequenceControlsConstants;
        g.fillAll (juce::Colours::transparentBlack);

        juce::Font titleFont;
        if (interBoldFont != nullptr)
            titleFont = juce::Font (interBoldFont);

        titleFont.setHeight (titleFontSize);

        g.setFont (titleFont);
        g.setColour (juce::Colours::white);

        auto area = getLocalBounds();
        auto titleArea = area.removeFromTop (titleHeight);
        titleArea.translate (0, UIConstants::titleYOffset);
        g.drawText ("Sub", titleArea, juce::Justification::centred, false);
    }

    void resized() override {
        using namespace SubsequenceControlsConstants;

        int barY = titleHeight + gapBelowTitle;

        // Center the bar horizontally
        int barX = (getWidth() - static_cast<int>(barWidth)) / 2;
        subsequenceBar.setBounds (barX, barY, static_cast<int>(barWidth), barStackHeight);

        // Position buttons below the bar
        int buttonsY = subsequenceBar.getBottom() + gapAboveButtons;

        // Calculate button positions so they align with bar edges
        // Left edge of "-" aligns with left edge of bar
        // Right edge of "+" aligns with right edge of bar
        int buttonWidthInt = static_cast<int>(std::ceil(buttonWidth));
        int totalButtonsWidth = 2 * buttonWidthInt;
        int spacing = static_cast<int>(barWidth) - totalButtonsWidth;

        minusButton.setBounds (barX, buttonsY, buttonWidthInt, buttonHeight);
        plusButton.setBounds (barX + buttonWidthInt + spacing, buttonsY, buttonWidthInt, buttonHeight);
    }

    // Access to the bar for external updates
    SubsequenceBar& getSubsequenceBar() { return subsequenceBar; }
    const SubsequenceBar& getSubsequenceBar() const { return subsequenceBar; }

    // Callbacks
    std::function<void(int)> onBarClicked;
    std::function<void()> onMinusClicked;
    std::function<void()> onPlusClicked;

private:
    juce::Typeface::Ptr interBoldFont;
    SubsequenceBar subsequenceBar;
    SubsequenceButton minusButton;
    SubsequenceButton plusButton;
};

