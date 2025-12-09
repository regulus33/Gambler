#pragma once

#include "BinaryData.h"
#include "NotchedSlider.h"
#include "MusicConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MainControlsComponent
 *
 * Top-level container for the Main Controls UI.
 * - Title at the top
 * - NotchedSlider for note selection
 */
class MainControlsComponent : public juce::Component {
public:
    MainControlsComponent() {
        // Load Inter font for the title (kept consistent with the rest of the UI)
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Create the note notched sliders using shared music/UI constants
        auto noteNames     = MusicConstants::toJuceArray (MusicConstants::CHROMATIC_NOTE_NAMES);
        auto noteColours   = MusicConstants::toJuceArray (MusicConstants::NOTE_COLOURS);
        auto noteOctaves   = MusicConstants::toJuceArray (MusicConstants::OCTAVE_LABELS);
        auto octaveColours = MusicConstants::toJuceArray (MusicConstants::OCTAVE_COLOURS);

        noteSlider = std::make_unique<NotchedSlider> ("Oct", noteOctaves, octaveColours);
        addAndMakeVisible (*noteSlider);

        noteSlider2 = std::make_unique<NotchedSlider> ("Note", noteNames, noteColours);
        addAndMakeVisible (*noteSlider2);
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

        // Container for note / octave sliders
        if (! noteContainerBounds.isEmpty()) {
            auto r = noteContainerBounds.toFloat();

            // Background fill inside the container
            g.setColour (juce::Colour (0xff2c2c2c));
            g.fillRoundedRectangle (r, 3.0f);

            // 1px white border with 3px corner radius
            g.setColour (juce::Colours::white);
            g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
        }
    }

    void resized() override {
        constexpr int titleHeight = 24;
        constexpr int gapBelowTitle = 12;
        constexpr int containerWidth = 188;
        constexpr int containerHeight = 101;
        constexpr int containerLeftMargin = 4;
        constexpr int containerInnerPadding = 4;

        auto bounds = getLocalBounds();

        // Fixed-size container positioned 12px below the title
        noteContainerBounds = juce::Rectangle<int> (
            containerLeftMargin,
            titleHeight + gapBelowTitle,
            containerWidth,
            containerHeight);

        // Layout sliders in two equal-width columns inside the container
        auto slidersArea = noteContainerBounds.reduced (containerInnerPadding);
        auto leftColumn = slidersArea.removeFromLeft (slidersArea.getWidth() / 2);
        auto rightColumn = slidersArea;

        if (noteSlider != nullptr)
            noteSlider->setBounds (leftColumn);

        if (noteSlider2 != nullptr)
            noteSlider2->setBounds (rightColumn);
    }

private:
    juce::Typeface::Ptr interFont;
    std::unique_ptr<NotchedSlider> noteSlider;
    std::unique_ptr<NotchedSlider> noteSlider2;
    juce::Rectangle<int> noteContainerBounds;
};

