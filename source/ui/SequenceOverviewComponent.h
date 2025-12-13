#pragma once

#include "BinaryData.h"
#include "SequenceGrid.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace SequenceOverviewConstants {
    // Title
    constexpr float titleFontSize = 16.0f;
    constexpr int titleHeight = 24;
    constexpr int gapBelowTitle = 13;
    // Container layout
    constexpr int containerWidth = 198;
    // Grid positioning (centered within container)
    constexpr int gridHorizontalPadding = (containerWidth - SequenceGridConstants::gridWidth) / 2;  // ~20px each side
    // Buttons - sized to fit within 80% of grid width
    constexpr int buttonWidth = 38;
    constexpr int buttonHeight = 20;
    constexpr int buttonSpacing = 4;
    constexpr int gapAboveButtons = 16;
    constexpr float buttonCornerRadius = 3.0f;
    constexpr float buttonFontSize = 10.0f;
    constexpr juce::uint32 buttonBgHex = 0xff484848;
    // Total buttons width: 3 * 38 + 2 * 4 = 122px (within 80% of 157px = 125.6px)
}

/**
 * SequenceButton - Simple button for Export/Save/Load actions
 */
class SequenceButton : public juce::Component {
public:
    explicit SequenceButton (const juce::String& labelText)
        : label (labelText)
    {
        interTypeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);
    }

    void paint (juce::Graphics& g) override {
        using namespace SequenceOverviewConstants;
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
 * SequenceOverviewComponent
 *
 * Top-level container for the Sequence Overview UI.
 * - Title "Sequence" at the top
 * - SequenceGrid (8x8 grid of colored squares)
 * - Three buttons: Export, Save, Load
 */
class SequenceOverviewComponent : public juce::Component {
public:
    SequenceOverviewComponent()
        : interFont (juce::Typeface::createSystemTypefaceFor (
              BinaryData::InterVariableFont_ttf,
              BinaryData::InterVariableFont_ttfSize)),
          exportButton ("Export"),
          saveButton ("Save"),
          loadButton ("Load")
    {
        addAndMakeVisible (sequenceGrid);
        addAndMakeVisible (exportButton);
        addAndMakeVisible (saveButton);
        addAndMakeVisible (loadButton);

        // Setup button callbacks
        exportButton.onClick = [this]() {
            if (onExportClicked)
                onExportClicked();
        };

        saveButton.onClick = [this]() {
            if (onSaveClicked)
                onSaveClicked();
        };

        loadButton.onClick = [this]() {
            if (onLoadClicked)
                onLoadClicked();
        };
    }

    void paint (juce::Graphics& g) override {
        using namespace SequenceOverviewConstants;
        g.fillAll (juce::Colours::transparentBlack);

        juce::Font titleFont;
        if (interFont != nullptr)
            titleFont = juce::Font (interFont);

        titleFont.setHeight (titleFontSize);
        titleFont.setBold (true);

        g.setFont (titleFont);
        g.setColour (juce::Colours::white);

        auto area = getLocalBounds();
        auto titleArea = area.removeFromTop (titleHeight);
        g.drawText ("Sequence", titleArea, juce::Justification::centred, false);
    }

    void resized() override {
        using namespace SequenceOverviewConstants;

        int gridY = titleHeight + gapBelowTitle;

        // Center the grid horizontally
        sequenceGrid.setBounds (
            gridHorizontalPadding,
            gridY,
            SequenceGridConstants::gridWidth,
            SequenceGridConstants::gridHeight);

        // Position buttons below the grid
        int buttonsY = sequenceGrid.getBottom() + gapAboveButtons;

        // Calculate total buttons width: 3 buttons + 2 gaps
        int totalButtonsWidth = 3 * buttonWidth + 2 * buttonSpacing;
        int buttonsStartX = (containerWidth - totalButtonsWidth) / 2;

        exportButton.setBounds (buttonsStartX, buttonsY, buttonWidth, buttonHeight);
        saveButton.setBounds (buttonsStartX + buttonWidth + buttonSpacing, buttonsY, buttonWidth, buttonHeight);
        loadButton.setBounds (buttonsStartX + 2 * (buttonWidth + buttonSpacing), buttonsY, buttonWidth, buttonHeight);
    }

    // Access to the grid for external updates
    SequenceGrid& getSequenceGrid() { return sequenceGrid; }
    const SequenceGrid& getSequenceGrid() const { return sequenceGrid; }

    // Button callbacks
    std::function<void()> onExportClicked;
    std::function<void()> onSaveClicked;
    std::function<void()> onLoadClicked;

private:
    juce::Typeface::Ptr interFont;
    SequenceGrid sequenceGrid;
    SequenceButton exportButton;
    SequenceButton saveButton;
    SequenceButton loadButton;
};

