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
        // Initialize with full chromatic scale (0-11)
        for (int i = 0; i < 12; i++) {
            scaleNoteIndices.add(i);
        }

        addAndMakeVisible (octaveSlider);
        addAndMakeVisible (noteSlider);

        // Wire up slider callbacks (also repaint the NotchedSlider to update gauge)
        octaveSlider.getSlider().onValueChange = [this] {
            octaveSlider.repaint();
            if (onOctaveChanged)
                onOctaveChanged(octaveSlider.getSelectedIndex());
        };

        noteSlider.getSlider().onValueChange = [this] {
            noteSlider.repaint();
            if (onNoteChanged)
                onNoteChanged(getChromaticPitchClass());
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
        setNoteByChromaticPitchClass(pitchClass);
    }

    /**
     * Set the current octave display (0-9 for octaves 1-10)
     */
    void setOctave(int octaveIndex) {
        if (octaveIndex >= 0 && octaveIndex < 10) {
            octaveSlider.getSlider().setValue(static_cast<double>(octaveIndex), juce::dontSendNotification);
            octaveSlider.repaint();  // Update the gauge display
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

    /**
     * Update the note slider to only show notes from the selected scale
     * @param noteNames Array of note names to display (e.g., {"C", "D", "E"})
     * @param noteColours Array of colours for each note (must match noteNames size)
     * @param noteIndices Array of chromatic indices (0-11) for each note, used for mapping
     */
    void updateNoteOptions(const juce::Array<juce::String>& noteNames,
                          const juce::Array<juce::Colour>& noteColours,
                          const juce::Array<int>& noteIndices) {
        if (noteNames.isEmpty() || noteNames.size() != noteColours.size() ||
            noteNames.size() != noteIndices.size()) {
            return; // Invalid input
        }

        // Store the mapping from slider index to chromatic pitch class
        scaleNoteIndices = noteIndices;

        // Update the note slider with the new options
        noteSlider.updateOptions(noteNames, noteColours);
    }

    /**
     * Get the chromatic pitch class (0-11) for the currently selected note
     * This accounts for the scale filtering
     */
    int getChromaticPitchClass() const {
        int sliderIndex = noteSlider.getSelectedIndex();
        if (sliderIndex >= 0 && sliderIndex < scaleNoteIndices.size()) {
            return scaleNoteIndices[sliderIndex];
        }
        // Fallback to direct index if no scale mapping exists
        return sliderIndex;
    }

    /**
     * Set the note by chromatic pitch class (0-11)
     * This will find the corresponding slider index based on the current scale
     */
    void setNoteByChromaticPitchClass(int chromaticPitchClass) {
        // Find the slider index that corresponds to this chromatic pitch class
        for (int i = 0; i < scaleNoteIndices.size(); i++) {
            if (scaleNoteIndices[i] == chromaticPitchClass) {
                noteSlider.getSlider().setValue(static_cast<double>(i), juce::dontSendNotification);
                noteSlider.repaint();
                return;
            }
        }
        // If not found, default to first note in scale
        if (!scaleNoteIndices.isEmpty()) {
            noteSlider.getSlider().setValue(0.0, juce::dontSendNotification);
            noteSlider.repaint();
        }
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

    // Mapping from note slider index to chromatic pitch class (0-11)
    // This allows the slider to show only scale notes while maintaining correct pitch mapping
    juce::Array<int> scaleNoteIndices;
};

