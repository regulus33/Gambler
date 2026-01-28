#pragma once

#include "../NotchedSlider.h"
#include "../MusicConstants.h"
#include "../../backend/MidiLearnRegistry.h"
#include "../../backend/MidiMapping.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * NoteSlider
 * 
 * Discrete component for note/pitch class selection (0-11, C to B).
 * Wraps NotchedSlider and can be individually MIDI mapped.
 * Supports scale filtering - can display only notes from the selected scale.
 */
class NoteSlider : public juce::Component {
public:
    NoteSlider()
        : slider_("Note",
                 MusicConstants::toJuceArray(MusicConstants::CHROMATIC_NOTE_NAMES),
                 MusicConstants::toJuceArray(MusicConstants::NOTE_COLOURS))
    {
        // Initialize with full chromatic scale (0-11)
        for (int i = 0; i < 12; i++) {
            scaleNoteIndices_.add(i);
        }

        addAndMakeVisible(slider_);
        
        // Wire up slider callback
        slider_.getSlider().onValueChange = [this] {
            slider_.repaint();
            if (onNoteChanged) {
                onNoteChanged(getChromaticPitchClass());
            }
        };
    }

    /**
     * Callback when note changes
     * Parameter: chromatic pitch class (0-11, C to B)
     */
    std::function<void(int)> onNoteChanged;

    /**
     * Set the current note by chromatic pitch class (0-11)
     * This will find the corresponding slider index based on the current scale
     */
    void setNote(int chromaticPitchClass) {
        // Find the slider index that corresponds to this chromatic pitch class
        for (int i = 0; i < scaleNoteIndices_.size(); i++) {
            if (scaleNoteIndices_[i] == chromaticPitchClass) {
                slider_.getSlider().setValue(static_cast<double>(i), juce::dontSendNotification);
                slider_.repaint();
                return;
            }
        }
        // If not found, default to first note in scale
        if (!scaleNoteIndices_.isEmpty()) {
            slider_.getSlider().setValue(0.0, juce::dontSendNotification);
            slider_.repaint();
        }
    }

    /**
     * Get the chromatic pitch class (0-11) for the currently selected note
     * This accounts for the scale filtering
     */
    int getChromaticPitchClass() const {
        int sliderIndex = slider_.getSelectedIndex();
        if (sliderIndex >= 0 && sliderIndex < scaleNoteIndices_.size()) {
            return scaleNoteIndices_[sliderIndex];
        }
        // Fallback to direct index if no scale mapping exists
        return sliderIndex;
    }

    /**
     * Get the currently selected slider index (not the chromatic pitch class)
     */
    int getSelectedIndex() const {
        return slider_.getSelectedIndex();
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
        scaleNoteIndices_ = noteIndices;

        // Update the note slider with the new options
        slider_.updateOptions(noteNames, noteColours);
    }

    void resized() override {
        slider_.setBounds(getLocalBounds());
    }

    // Accessor for testing
    NotchedSlider& getSlider() { return slider_; }

private:
    NotchedSlider slider_;
    
    // Mapping from note slider index to chromatic pitch class (0-11)
    // This allows the slider to show only scale notes while maintaining correct pitch mapping
    juce::Array<int> scaleNoteIndices_;
};

