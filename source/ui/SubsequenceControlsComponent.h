#pragma once

#include "BinaryData.h"
#include "MusicConstants.h"
#include "SequenceGrid.h"  // For SquareState and constants
#include "StateListenerComponent.h"
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
    constexpr float barWidth = 50.f;
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
 * SingleBar - Individual clickable rectangle component
 */
class SingleBar : public juce::Component {
public:
    SingleBar()
        : color (juce::Colour (SubsequenceControlsConstants::defaultGreyHex)),
          hasBorder (false)
    {
    }

    void paint (juce::Graphics& g) override {
        using namespace SubsequenceControlsConstants;
        auto bounds = getLocalBounds().toFloat();

        // Fill bar with color
        g.setColour (color);
        g.fillRoundedRectangle (bounds, barCornerRadius);

        // Draw white border if enabled
        if (hasBorder) {
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (bounds.reduced (0.5f), barCornerRadius, borderWidth);
        }
    }

    void mouseDown (const juce::MouseEvent& /*event*/) override {
        if (onClick)
            onClick();
    }

    void setColor (juce::Colour newColor) {
        color = newColor;
        repaint();
    }

    juce::Colour getColor() const {
        return color;
    }

    void setBorder (bool shouldHaveBorder) {
        hasBorder = shouldHaveBorder;
        repaint();
    }

    bool getBorder() const {
        return hasBorder;
    }

    std::function<void()> onClick;

private:
    juce::Colour color;
    bool hasBorder;
};

/**
 * SubsequenceBar - A vertical stack of 8 clickable rectangle components
 */
class SubsequenceBar : public juce::Component {
public:
    SubsequenceBar() {
        // Create and add all bar components
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            bars[i] = std::make_unique<SingleBar>();
            addAndMakeVisible (bars[i].get());

            // Setup click callback for each bar
            bars[i]->onClick = [this, i]() {
                if (onBarClicked)
                    onBarClicked (i);
            };
        }
    }

    void resized() override {
        using namespace SubsequenceControlsConstants;

        for (int i = 0; i < numBars; ++i) {
            int y = i * (barHeight + barSpacing);
            bars[i]->setBounds (0, y, static_cast<int>(barWidth), barHeight);
        }
    }

    /**
     * Set the color of a single bar
     * @param index Bar index (0-7)
     * @param color The new color
     */
    void setBarColor (int index, juce::Colour color) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setColor (color);
        }
    }

    /**
     * Get the color of a single bar
     * @param index Bar index (0-7)
     */
    juce::Colour getBarColor (int index) const {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index]->getColor();
        }
        return juce::Colour (SubsequenceControlsConstants::defaultGreyHex);
    }

    /**
     * Set the border visibility of a single bar
     * @param index Bar index (0-7)
     * @param hasBorder Whether to show the border
     */
    void setBarBorder (int index, bool hasBorder) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            bars[index]->setBorder (hasBorder);
        }
    }

    /**
     * Get the border state of a single bar
     * @param index Bar index (0-7)
     */
    bool getBarBorder (int index) const {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index]->getBorder();
        }
        return false;
    }

    /**
     * Clear all borders
     */
    void clearAllBorders() {
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            bars[i]->setBorder (false);
        }
    }

    /**
     * Get direct access to a bar component
     * @param index Bar index (0-7)
     */
    SingleBar* getBar (int index) {
        if (index >= 0 && index < SubsequenceControlsConstants::numBars) {
            return bars[index].get();
        }
        return nullptr;
    }

    // Callback for bar clicks
    std::function<void(int)> onBarClicked;

private:
    std::array<std::unique_ptr<SingleBar>, SubsequenceControlsConstants::numBars> bars;
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
 *
 * Observer Pattern:
 * - Listens to selected step and substep parameters
 * - Automatically updates UI when substep data changes
 */
class SubsequenceControlsComponent : public StateListenerComponent {
public:
    explicit SubsequenceControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor),
          interBoldFont (juce::Typeface::createSystemTypefaceFor (
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

        // Subscribe to selected step and all substep parameters
        listenToParameter(Params::Keys::ID::SEQ_SELECTED_STEP);
        listenToStepNotes(0, ParamConstants::MAX_STEPS - 1);
        listenToStepNotesLength(0, ParamConstants::MAX_STEPS - 1);
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
        g.drawText ("Subseq", titleArea, juce::Justification::centred, false);
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

protected:
    /**
     * Called on message thread when any subscribed parameter changes.
     * Updates the subsequence bar to reflect the current substep state.
     */
    void onStateChanged() override {
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        int substepLength = state.getStepNotesLength(selectedStep);

        // Update bar colors and borders based on substep data
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            if (i < substepLength) {
                // Get the note for this substep and determine its color
                int noteNumber = state.getStepNote(selectedStep, i);
                int pitchClass = noteNumber % 12;
                juce::Colour noteColor = MusicConstants::NOTE_COLOURS[static_cast<size_t>(pitchClass)];
                subsequenceBar.setBarColor(i, noteColor);
            } else {
                // Inactive substep - grey
                subsequenceBar.setBarColor(i, juce::Colour(SubsequenceControlsConstants::defaultGreyHex));
            }

            // Clear all borders - the current substep border is handled by playback
            subsequenceBar.setBarBorder(i, false);
        }
    }

private:
    juce::Typeface::Ptr interBoldFont;
    SubsequenceBar subsequenceBar;
    SubsequenceButton minusButton;
    SubsequenceButton plusButton;
};

