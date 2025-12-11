#pragma once

#include "BinaryData.h"
#include "NoteControlsComponent.h"
#include "ProbabilityControlsComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

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
        g.fillAll (juce::Colours::transparentBlack);

        // Title
        juce::Font titleFont;
        if (interFont != nullptr)
            titleFont = juce::Font (interFont);

        titleFont.setHeight (16.0f);
        titleFont.setBold (true);

        g.setFont (titleFont);
        g.setColour (juce::Colours::white);

        auto area = getLocalBounds();
        auto titleArea = area.removeFromTop (24);
        g.drawText ("Main Controls", titleArea, juce::Justification::centred, false);
    }

    void resized() override {
        constexpr int titleHeight = 24;
        constexpr int gapBelowTitle = 6;
        // Width matches 2 sequencer steps (2×92) + 1 gap (1×18) = 202px
        constexpr int containerWidth = 198;
        constexpr int containerHeight = 101;
        constexpr int containerLeftMargin = 4;
        constexpr int gapBetweenContainers = 10;

        noteControls.setBounds (
            containerLeftMargin,
            titleHeight + gapBelowTitle,
            containerWidth,
            containerHeight);

        probabilityControls.setBounds (
            containerLeftMargin,
            noteControls.getBottom() + gapBetweenContainers,
            containerWidth,
            containerHeight);
    }

private:
    juce::Typeface::Ptr interFont;
    NoteControlsComponent noteControls;
    ProbabilityControlsComponent probabilityControls;
};

