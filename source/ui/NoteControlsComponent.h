#pragma once

#include "NotchedSlider.h"
#include "MusicConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * NoteControlsComponent
 *
 * A bordered container with NotchedSliders for note and octave selection.
 * Displays a 1px white border with 3px corner radius around the sliders.
 */
class NoteControlsComponent : public juce::Component {
public:
    NoteControlsComponent()
        : octaveSlider ("Oct",
                        MusicConstants::toJuceArray (MusicConstants::OCTAVE_LABELS),
                        MusicConstants::toJuceArray (MusicConstants::OCTAVE_COLOURS)),
          noteSlider ("Note",
                      MusicConstants::toJuceArray (MusicConstants::CHROMATIC_NOTE_NAMES),
                      MusicConstants::toJuceArray (MusicConstants::NOTE_COLOURS))
    {
        addAndMakeVisible (octaveSlider);
        addAndMakeVisible (noteSlider);
    }

    void paint (juce::Graphics& g) override {
        g.fillAll (juce::Colours::transparentBlack);

        // 1px white border with 3px corner radius
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    }

    void resized() override {
        constexpr int innerPadding = 4;

        // Layout sliders in two equal-width columns inside the container
        auto slidersArea = getLocalBounds().reduced (innerPadding);
        auto leftColumn = slidersArea.removeFromLeft (slidersArea.getWidth() / 2);
        auto rightColumn = slidersArea;

        octaveSlider.setBounds (leftColumn);
        noteSlider.setBounds (rightColumn);
    }

private:
    NotchedSlider octaveSlider;
    NotchedSlider noteSlider;
};

