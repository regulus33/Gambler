#pragma once

#include "NotchedSlider.h"
#include "MusicConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace NoteControlsConstants {
    constexpr float borderStrokeWidth = 2.5f;
    constexpr float cornerRadius = 3.0f;
    constexpr int innerPadding = 4;
}

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

        // Wire up slider callbacks
        octaveSlider.getSlider().onValueChange = [this] {
            if (onOctaveChanged)
                onOctaveChanged(octaveSlider.getSelectedIndex());
        };

        noteSlider.getSlider().onValueChange = [this] {
            if (onNoteChanged)
                onNoteChanged(noteSlider.getSelectedIndex());
        };
    }

    /**
     * Callback when octave slider changes
     * Parameter: octave index (0-9, representing octaves 1-10)
     */
    std::function<void(int)> onOctaveChanged;

    /**
     * Callback when note slider changes
     * Parameter: pitch class (0-11, representing C to B)
     */
    std::function<void(int)> onNoteChanged;

    /**
     * Set the current note display (pitch class 0-11)
     */
    void setNote(int pitchClass) {
        if (pitchClass >= 0 && pitchClass < 12) {
            noteSlider.getSlider().setValue(static_cast<double>(pitchClass), juce::dontSendNotification);
        }
    }

    /**
     * Set the current octave display (0-9 for octaves 1-10)
     */
    void setOctave(int octaveIndex) {
        if (octaveIndex >= 0 && octaveIndex < 10) {
            octaveSlider.getSlider().setValue(static_cast<double>(octaveIndex), juce::dontSendNotification);
        }
    }

    /**
     * Set both note and octave from a MIDI note number (0-127)
     */
    void setFromMidiNote(int midiNote) {
        if (midiNote >= 0 && midiNote <= 127) {
            int pitchClass = midiNote % 12;
            int octave = midiNote / 12;  // MIDI octave (0-10)
            setNote(pitchClass);
            setOctave(octave);
        }
    }

    /**
     * Get the current MIDI note number from the sliders
     */
    int getMidiNote() const {
        int pitchClass = noteSlider.getSelectedIndex();
        int octave = octaveSlider.getSelectedIndex();
        return octave * 12 + pitchClass;
    }

    void paint (juce::Graphics& g) override {
        using namespace NoteControlsConstants;
        g.fillAll (juce::Colours::transparentBlack);

        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (r.reduced (0.5f), cornerRadius, borderStrokeWidth);
    }

    void resized() override {
        using namespace NoteControlsConstants;

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

