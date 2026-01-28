#pragma once

#include "../NotchedSlider.h"
#include "../MusicConstants.h"
#include "../../backend/MidiLearnRegistry.h"
#include "../../backend/MidiMapping.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * OctaveSlider
 * 
 * Discrete component for octave selection (0-9, representing octaves 1-10).
 * Wraps NotchedSlider and can be individually MIDI mapped.
 */
class OctaveSlider : public juce::Component {
public:
    OctaveSlider()
        : slider_("Oct",
                 MusicConstants::toJuceArray(MusicConstants::OCTAVE_LABELS),
                 MusicConstants::toJuceArray(MusicConstants::OCTAVE_COLOURS))
    {
        addAndMakeVisible(slider_);
        
        // Wire up slider callback
        slider_.getSlider().onValueChange = [this] {
            slider_.repaint();
            if (onOctaveChanged) {
                onOctaveChanged(getSelectedIndex());
            }
        };
    }

    /**
     * Callback when octave changes
     * Parameter: octave index (0-9, representing octaves 1-10)
     */
    std::function<void(int)> onOctaveChanged;

    /**
     * Set the current octave (0-9)
     */
    void setOctave(int octaveIndex) {
        if (octaveIndex >= 0 && octaveIndex < 10) {
            slider_.getSlider().setValue(static_cast<double>(octaveIndex), juce::dontSendNotification);
            slider_.repaint();
        }
    }

    /**
     * Get the currently selected octave index (0-9)
     */
    int getSelectedIndex() const {
        return slider_.getSelectedIndex();
    }

    void resized() override {
        slider_.setBounds(getLocalBounds());
    }

    // Accessor for testing
    NotchedSlider& getSlider() { return slider_; }

private:
    NotchedSlider slider_;
};

