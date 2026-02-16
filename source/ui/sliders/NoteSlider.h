#pragma once

#include "../../backend/MidiLearnRegistry.h"
#include "../../backend/MidiMapping.h"
#include "../MidiLearnOverlayPainter.h"
#include "../MusicConstants.h"
#include "NotchedSlider.h"
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
                 MusicConstants::toJuceArray(MusicConstants::NOTE_COLOURS)),
          midiControlId_(MidiControlId::NOTE_SLIDER)
    {
        // Initialize with full chromatic scale (0-11)
        for (int i = 0; i < 12; i++) {
            scaleNoteIndices_.add(i);
        }

        addAndMakeVisible(slider_);

        // Add invisible overlay for MIDI learn mode (sits on top, initially hidden)
        addAndMakeVisible(midiLearnOverlay_);
        midiLearnOverlay_.setVisible(false);
        midiLearnOverlay_.setInterceptsMouseClicks(true, false);
        midiLearnOverlay_.onMouseDown = [this]() {
            if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
                DBG("NoteSlider overlay clicked: Setting learning control to " << static_cast<int>(midiControlId_));
                registry_->setLearningControl(midiControlId_);
                DBG("NoteSlider: Learning control is now " << static_cast<int>(registry_->getLearningControl()));

                // Notify that this control was selected (triggers repaint of all MIDI learn components)
                if (onMidiLearnSelected) {
                    onMidiLearnSelected();
                }
            }
        };

        // Wire up slider callback
        slider_.getSlider().onValueChange = [this] {
            slider_.repaint();
            if (onNoteChanged) {
                onNoteChanged(getChromaticPitchClass());
            }
        };
    }

    /**
     * Set the MIDI learn registry for this slider
     */
    void setMidiLearnRegistry(MidiLearnRegistry* registry) {
        registry_ = registry;
    }

    /**
     * Callback when this control is selected for MIDI learning
     * Use this to trigger repainting of all MIDI learn components
     */
    std::function<void()> onMidiLearnSelected;

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
     * Update the full set of 12 note colours (called when note colour params change)
     * This updates the slider's internal colour array for the chromatic scale
     */
    void updateNoteColours(const juce::Array<juce::Colour>& allColours) {
        if (allColours.size() == 12) {
            allNoteColours_ = allColours;
            // Rebuild current options using stored scale indices
            juce::Array<juce::Colour> filteredColours;
            for (int idx : scaleNoteIndices_) {
                filteredColours.add(allNoteColours_[idx]);
            }
            juce::Array<juce::String> filteredNames;
            for (int idx : scaleNoteIndices_) {
                filteredNames.add(MusicConstants::CHROMATIC_NOTE_NAMES[static_cast<size_t>(idx)]);
            }
            slider_.updateOptions(filteredNames, filteredColours);
        }
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

    void paint(juce::Graphics& g) override {
        // Paint MIDI learn overlay if in learn mode
        if (registry_ && registry_->isLearning()) {
            bool isLearningThis = (registry_->getLearningControl() == midiControlId_);
            DBG("NoteSlider paint: isLearning=" << (int)registry_->isLearning()
                << " isLearningThis=" << (int)isLearningThis
                << " controlId=" << static_cast<int>(midiControlId_));

            // Show the overlay when in learn mode
            midiLearnOverlay_.setVisible(true);
        } else {
            // Hide the overlay when not in learn mode
            midiLearnOverlay_.setVisible(false);
        }
        paintMidiLearnOverlay(g);
    }

    void resized() override {
        slider_.setBounds(getLocalBounds());
        midiLearnOverlay_.setBounds(getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent& event) override {
        // Handle MIDI learn mode
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
            DBG("NoteSlider: Setting learning control to " << static_cast<int>(midiControlId_));
            registry_->setLearningControl(midiControlId_);
            DBG("NoteSlider: Learning control is now " << static_cast<int>(registry_->getLearningControl()));
            repaint();
            return;
        }

        // Forward to slider
        slider_.mouseDown(event.withNewPosition(event.position - slider_.getPosition().toFloat()));
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        // Forward to slider (unless in MIDI learn mode)
        if (registry_ && registry_->isLearning()) return;
        slider_.mouseDrag(event.withNewPosition(event.position - slider_.getPosition().toFloat()));
    }

    void mouseUp(const juce::MouseEvent& event) override {
        // Forward to slider (unless in MIDI learn mode)
        if (registry_ && registry_->isLearning()) return;
        slider_.mouseUp(event.withNewPosition(event.position - slider_.getPosition().toFloat()));
    }

    // Accessor for testing
    NotchedSlider& getSlider() { return slider_; }

private:
    void paintMidiLearnOverlay(juce::Graphics& g) {
        // No corner radius for sliders (they're circular knobs)
        MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, 0.0f);
    }

    // Transparent overlay component that sits on top of the slider in MIDI learn mode
    class MidiLearnOverlay : public juce::Component {
    public:
        std::function<void()> onMouseDown;

        void mouseDown(const juce::MouseEvent&) override {
            if (onMouseDown) {
                onMouseDown();
            }
        }

        void paint(juce::Graphics&) override {
            // Transparent - the parent will paint the overlay visuals
        }
    };

    NotchedSlider slider_;
    MidiLearnOverlay midiLearnOverlay_;

    // Mapping from note slider index to chromatic pitch class (0-11)
    // This allows the slider to show only scale notes while maintaining correct pitch mapping
    juce::Array<int> scaleNoteIndices_;

    // Full set of 12 note colours (updated dynamically from APVTS)
    juce::Array<juce::Colour> allNoteColours_;

    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;
};

