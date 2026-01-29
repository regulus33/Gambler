#pragma once

#include "../NotchedSlider.h"
#include "../MusicConstants.h"
#include "../MidiLearnOverlayPainter.h"
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
                 MusicConstants::toJuceArray(MusicConstants::OCTAVE_COLOURS)),
          midiControlId_(MidiControlId::OCTAVE_SLIDER)
    {
        addAndMakeVisible(slider_);

        // Add invisible overlay for MIDI learn mode (sits on top, initially hidden)
        addAndMakeVisible(midiLearnOverlay_);
        midiLearnOverlay_.setVisible(false);
        midiLearnOverlay_.setInterceptsMouseClicks(true, false);
        midiLearnOverlay_.onMouseDown = [this]() {
            if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
                DBG("OctaveSlider overlay clicked: Setting learning control to " << static_cast<int>(midiControlId_));
                registry_->setLearningControl(midiControlId_);
                DBG("OctaveSlider: Learning control is now " << static_cast<int>(registry_->getLearningControl()));

                // Notify that this control was selected (triggers repaint of all MIDI learn components)
                if (onMidiLearnSelected) {
                    onMidiLearnSelected();
                }
            }
        };

        // Wire up slider callback
        slider_.getSlider().onValueChange = [this] {
            slider_.repaint();
            if (onOctaveChanged) {
                onOctaveChanged(getSelectedIndex());
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

    void paint(juce::Graphics& g) override {
        // Paint MIDI learn overlay if in learn mode
        if (registry_ && registry_->isLearning()) {
            bool isLearningThis = (registry_->getLearningControl() == midiControlId_);
            DBG("OctaveSlider paint: isLearning=" << (int)registry_->isLearning()
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
            DBG("OctaveSlider: Setting learning control to " << static_cast<int>(midiControlId_));
            registry_->setLearningControl(midiControlId_);
            DBG("OctaveSlider: Learning control is now " << static_cast<int>(registry_->getLearningControl()));
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
    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;
};

