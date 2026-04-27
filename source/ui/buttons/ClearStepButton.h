#pragma once

#include "SequencerControlButton.h"

/**
 * ClearStepButton - Erase button on the sequencer control row.
 *
 * - Single tap: clears only the *selected* substep of the selected step
 *   (sets it to a rest).
 * - Double tap: wipes every substep slot of the selected step and resets
 *   its substep length to 1, leaving an empty step.
 *
 * Both actions are surfaced as APVTS triggers (CLEAR_STEP and
 * CLEAR_STEP_ALL) so PluginProcessor::parameterChanged owns the actual
 * mutation. CLEAR_STEP shares the BUTTON_LENGTH_PLUS MIDI mapping;
 * CLEAR_STEP_ALL is param-only for now (no spare MidiControlId slot).
 */
class ClearStepButton : public SequencerControlButton {
public:
    explicit ClearStepButton(PluginProcessor& processor)
        : SequencerControlButton(processor,
                                  Params::Keys::ID::CLEAR_STEP,
                                  MidiControlId::BUTTON_LENGTH_PLUS)
    {
        setIcon(Icons::createEraserIcon());
        setLabel("Erase");
    }

    void mouseDown(const juce::MouseEvent& event) override {
        if (auto* reg = getRegistry(); reg && reg->isLearning()) {
            SequencerControlButton::mouseDown(event);
            return;
        }
        if (event.getNumberOfClicks() >= 2) {
            doubleTapPending_ = true;
            fireClearAll(true);
            return;
        }
        SequencerControlButton::mouseDown(event);
    }

    void mouseUp(const juce::MouseEvent& event) override {
        if (doubleTapPending_) {
            doubleTapPending_ = false;
            fireClearAll(false);
            return;
        }
        SequencerControlButton::mouseUp(event);
    }

private:
    void fireClearAll(bool on) {
        if (auto* p = getProcessor().getAPVTS().getParameter(Params::Keys::ID::CLEAR_STEP_ALL))
            p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    }

    bool doubleTapPending_ = false;
};
