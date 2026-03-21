#pragma once

#include "../../backend/MidiLearnRegistry.h"
#include "../../backend/MidiMapping.h"
#include "../MidiLearnOverlayPainter.h"
#include "../MusicConstants.h"
#include "NotchedSlider.h"
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
        // Initialize with full octave range (0-9)
        for (int i = 0; i < 10; i++)
            octaveIndices_.add(i);

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
                onOctaveChanged(getActualOctaveIndex());
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
     * Set the current octave by actual octave index (0-9).
     * Finds the corresponding slider position based on the current octave options.
     */
    void setOctave(int octaveIndex) {
        for (int i = 0; i < octaveIndices_.size(); i++) {
            if (octaveIndices_[i] == octaveIndex) {
                slider_.getSlider().setValue(static_cast<double>(i), juce::dontSendNotification);
                slider_.repaint();
                return;
            }
        }
        // If not found, default to first available octave
        if (!octaveIndices_.isEmpty()) {
            slider_.getSlider().setValue(0.0, juce::dontSendNotification);
            slider_.repaint();
        }
    }

    /**
     * Get the actual octave index (0-9) for the currently selected slider position.
     * Accounts for filtered octave options.
     */
    int getActualOctaveIndex() const {
        int sliderIndex = slider_.getSelectedIndex();
        if (sliderIndex >= 0 && sliderIndex < octaveIndices_.size())
            return octaveIndices_[sliderIndex];
        return sliderIndex;
    }

    /**
     * Get the currently selected slider index (not the actual octave index).
     */
    int getSelectedIndex() const {
        return slider_.getSelectedIndex();
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
        if (octaveLabels.isEmpty() || octaveLabels.size() != octaveColours.size() ||
            octaveLabels.size() != octaveIndices.size())
            return;

        octaveIndices_ = octaveIndices;
        slider_.updateOptions(octaveLabels, octaveColours);
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

        if (slider_.isLocked()) return;

        // Forward to slider
        slider_.mouseDown(event.withNewPosition(event.position - slider_.getPosition().toFloat()));
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        if (registry_ && registry_->isLearning()) return;
        if (slider_.isLocked()) return;
        slider_.mouseDrag(event.withNewPosition(event.position - slider_.getPosition().toFloat()));
    }

    void mouseUp(const juce::MouseEvent& event) override {
        if (registry_ && registry_->isLearning()) return;
        if (slider_.isLocked()) return;
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

    // Mapping from slider index to actual octave index (0-9)
    // Allows the slider to show only reachable octaves while maintaining correct mapping
    juce::Array<int> octaveIndices_;

    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;
};

