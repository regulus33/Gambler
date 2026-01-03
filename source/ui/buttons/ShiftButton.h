#pragma once

#include "SequencerControlButton.h"

/**
 * ShiftButton - Toggles visibility of secondary button row
 *
 * Observer pattern implementation:
 * - mouseDown toggles SHIFT param (true <-> false)
 * - mouseUp does nothing (toggle persists)
 * - Visual state reflects SEQ_IS_SHIFT param (the actual state, not the button trigger)
 * - PluginProcessor::parameterChanged handles the state change
 *
 * This is a TOGGLE button - clicking toggles the state, which persists until clicked again.
 *
 * Note: This button observes SEQ_IS_SHIFT for visual state (the actual shift mode state),
 * but sets the SHIFT param on click (the button trigger). The PluginProcessor bridges these.
 */
class ShiftButton : public SequencerControlButton {
public:
    explicit ShiftButton(PluginProcessor& processor)
        : SequencerControlButton(processor,
                                  Params::Keys::ID::SEQ_IS_SHIFT,  // Observe the state param for visuals
                                  MidiControlId::BUTTON_SHIFT)
    {
        setIcon(Icons::createShiftIcon());
        setLabel("Shift");
    }

    // Override mouseDown to toggle the SHIFT button param
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

        // Toggle the SHIFT button param (not SEQ_IS_SHIFT - that's handled by PluginProcessor)
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::SHIFT)) {
            bool currentState = param->getValue() > 0.5f;
            param->setValueNotifyingHost(currentState ? 0.0f : 1.0f);
        }
    }

    // Override mouseUp to do nothing (toggle persists)
    void mouseUp(const juce::MouseEvent& event) override {
        juce::ignoreUnused(event);
        // Do nothing - toggle state persists
    }
};

