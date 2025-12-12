#pragma once

#include "BinaryData.h"
#include "NoteControlsComponent.h"
#include "ProbabilityControlsComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace MainControlsConstants {
    // Title
    constexpr float titleFontSize = 16.0f;
    constexpr int titleHeight = 24;
    constexpr int gapBelowTitle = 13;
    // Container layout
    constexpr int containerWidth = 198;
    constexpr int containerHeight = 100;
    constexpr int gapBetweenContainers = 8;
}

/**
 * MainControlsComponent
 *
 * Top-level container for the Main Controls UI.
 * - Title at the top
 * - NoteControlsComponent for note/octave selection
 * - ProbabilityControlsComponent for probability controls
 */
class MainControlsComponent : public juce::Component {
public:
    MainControlsComponent()
        : interFont (juce::Typeface::createSystemTypefaceFor (
              BinaryData::InterVariableFont_ttf,
              BinaryData::InterVariableFont_ttfSize))
    {
        addAndMakeVisible (noteControls);
        addAndMakeVisible (probabilityControls);
    }

    void paint (juce::Graphics& g) override {
        using namespace MainControlsConstants;
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
        g.drawText ("Main Controls", titleArea, juce::Justification::centred, false);
    }

    void resized() override {
        using namespace MainControlsConstants;

        noteControls.setBounds (
            0,
            titleHeight + gapBelowTitle,
            containerWidth,
            containerHeight);

        probabilityControls.setBounds (
            0,
            noteControls.getBottom() + gapBetweenContainers,
            containerWidth,
            ProbabilityControlsConstants::containerHeight);
    }

private:
    juce::Typeface::Ptr interFont;
    NoteControlsComponent noteControls;
    ProbabilityControlsComponent probabilityControls;
};

