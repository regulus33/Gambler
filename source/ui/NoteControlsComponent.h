#pragma once

#include "sliders/OctaveSlider.h"
#include "sliders/NoteSlider.h"
#include "BinaryData.h"
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
        : interBoldFont_(juce::Typeface::createSystemTypefaceFor(
              BinaryData::Inter_24ptBold_ttf,
              BinaryData::Inter_24ptBold_ttfSize))
    {
        setPaintingIsUnclipped(true);
        addAndMakeVisible(octaveSlider_);
        addAndMakeVisible(noteSlider_);

        // Wire up slider callbacks
        octaveSlider_.onOctaveChanged = [this](int octaveIndex) {
            if (onOctaveChanged) {
                onOctaveChanged(octaveIndex);
            }
        };

        noteSlider_.onNoteChanged = [this](int chromaticPitchClass) {
            if (onNoteChanged) {
                onNoteChanged(chromaticPitchClass);
            }
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
        noteSlider_.setNote(pitchClass);
    }

    /**
     * Set the current octave display (0-9 for octaves 1-10)
     */
    void setOctave(int octaveIndex) {
        octaveSlider_.setOctave(octaveIndex);
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
        int pitchClass = noteSlider_.getChromaticPitchClass();
        int octave = octaveSlider_.getActualOctaveIndex();
        return octave * 12 + pitchClass;
    }

    /**
     * Update the note slider to only show notes reachable within the current scale and range.
     * @param noteNames   Note names to display (e.g., {"C", "D", "E"})
     * @param noteColours Colours for each note (must match noteNames size)
     * @param noteIndices Chromatic indices (0-11) for each note, used for mapping
     */
    void updateNoteOptions(const juce::Array<juce::String>& noteNames,
                          const juce::Array<juce::Colour>& noteColours,
                          const juce::Array<int>& noteIndices) {
        noteSlider_.updateNoteOptions(noteNames, noteColours, noteIndices);
    }

    /**
     * Update the octave slider to only show octaves reachable within the current note range.
     * @param octaveLabels  Labels to display (e.g., {"3", "4", "5"})
     * @param octaveColours Colours for each label (must match octaveLabels size)
     * @param octaveIndices Actual octave indices (0-9) for each label, used for mapping
     */
    void updateOctaveOptions(const juce::Array<juce::String>& octaveLabels,
                             const juce::Array<juce::Colour>& octaveColours,
                             const juce::Array<int>& octaveIndices) {
        octaveSlider_.updateOctaveOptions(octaveLabels, octaveColours, octaveIndices);
    }

    /**
     * Set the MIDI learn registry for both sliders
     */
    void setRegistry(MidiLearnRegistry* registry) {
        octaveSlider_.setMidiLearnRegistry(registry);
        noteSlider_.setMidiLearnRegistry(registry);
    }

    /**
     * Set the callback for when a slider is selected for MIDI learning
     * This should trigger a repaint of all MIDI learn components
     */
    void setMidiLearnSelectedCallback(std::function<void()> callback) {
        octaveSlider_.onMidiLearnSelected = callback;
        noteSlider_.onMidiLearnSelected = callback;
    }

    void setRestOverlay(bool isRest) {
        if (showRestOverlay_ != isRest) {
            showRestOverlay_ = isRest;
            octaveSlider_.setVisible(!isRest);
            noteSlider_.setVisible(!isRest);
            repaint();
        }
    }

    void paint (juce::Graphics& g) override {
        using namespace NoteControlsConstants;
        g.fillAll (juce::Colours::transparentBlack);

        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (r.reduced (0.5f), cornerRadius, borderStrokeWidth);

        if (showRestOverlay_) {
            g.setColour(juce::Colours::white);
            g.setFont(juce::Font(juce::FontOptions(interBoldFont_).withHeight(20.0f)));
            g.drawText("REST", getLocalBounds(), juce::Justification::centred);
        }
    }

    void resized() override {
        using namespace NoteControlsConstants;

        // Layout sliders in two equal-width columns inside the container
        auto slidersArea = getLocalBounds().reduced(innerPadding);
        auto leftColumn = slidersArea.removeFromLeft(slidersArea.getWidth() / 2);
        auto rightColumn = slidersArea;

        octaveSlider_.setBounds(leftColumn);
        noteSlider_.setBounds(rightColumn);
    }

    // Accessors for testing and external access
    NoteSlider& getNoteSlider() { return noteSlider_; }
    OctaveSlider& getOctaveSlider() { return octaveSlider_; }

private:
    OctaveSlider octaveSlider_;
    NoteSlider noteSlider_;
    juce::Typeface::Ptr interBoldFont_;
    bool showRestOverlay_ = false;
};

