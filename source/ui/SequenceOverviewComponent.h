#pragma once

#include "BinaryData.h"
#include "SequenceGrid.h"
#include "UIConstants.h"
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
    constexpr int gridHorizontalPadding = (containerWidth - SequenceGridConstants::gridWidth) / 2;
}

/**
 * SequenceOverviewComponent
 *
 * Top-level container for the Sequence Overview UI.
 * - Title "Sequence" at the top
 * - SequenceGrid (8x8 grid of colored squares)
 */
class SequenceOverviewComponent : public juce::Component {
public:
    SequenceOverviewComponent()
        : interBoldFont (juce::Typeface::createSystemTypefaceFor (
              BinaryData::Inter_24ptBold_ttf,
              BinaryData::Inter_24ptBold_ttfSize))
    {
        addAndMakeVisible (sequenceGrid);
    }

    void paint (juce::Graphics& g) override {
        using namespace SequenceOverviewConstants;
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
    }

    // Access to the grid for external updates
    SequenceGrid& getSequenceGrid() { return sequenceGrid; }
    const SequenceGrid& getSequenceGrid() const { return sequenceGrid; }

private:
    juce::Typeface::Ptr interBoldFont;
    SequenceGrid sequenceGrid;
};

