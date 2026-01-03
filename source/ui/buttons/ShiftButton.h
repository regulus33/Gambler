#pragma once

#include "SequencerControlButton.h"

/**
 * ShiftButton - Toggles between normal and shift button modes
 *
 * Observer pattern implementation:
 * - mouseDown toggles SEQ_IS_SHIFT param directly (true <-> false)
 * - mouseUp does nothing (toggle persists)
 * - Visual state reflects SEQ_IS_SHIFT param value
 *
 * This is a TOGGLE button - clicking toggles the state, which persists until clicked again.
 * When shift is ON, buttons at positions 1-6 are replaced with alternate functions.
 */
class ShiftButton : public SequencerControlButton {
public:
    explicit ShiftButton(PluginProcessor& processor)
        : SequencerControlButton(processor,
                                  Params::Keys::ID::SEQ_IS_SHIFT,
                                  MidiControlId::BUTTON_SHIFT)
    {
        setIcon(Icons::createShiftIcon());
        setLabel("Shift");
    }

    // Override mouseDown to toggle SEQ_IS_SHIFT directly
    void mouseDown(const juce::MouseEvent& event) override {
        juce::ignoreUnused(event);

        // Handle MIDI learn mode first
        if (auto* reg = getRegistry()) {
            if (reg->isLearning()) {
                reg->setLearningControl(MidiControlId::BUTTON_SHIFT);
                repaint();
                return;
            }
        }

        // Toggle SEQ_IS_SHIFT directly
        setParamOn(!isParamOn());
    }

    // Override mouseUp to do nothing (toggle persists)
    void mouseUp(const juce::MouseEvent& event) override {
        juce::ignoreUnused(event);
        // Do nothing - toggle state persists
    }
};

