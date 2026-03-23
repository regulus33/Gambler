#pragma once

#include "SequencerControlButton.h"

/**
 * RollButton - Enables random mode while held (momentary), or locked on via swipe-up
 *
 * Interaction:
 * - Click and release: momentary roll (on while held, off on release)
 * - Click and swipe upward: locks roll on (dark maroon fill)
 * - Click again while locked: unlocks and turns roll off
 */
class RollButton : public SequencerControlButton {
public:
    explicit RollButton(PluginProcessor& processor)
        : SequencerControlButton(processor,
                                  Params::Keys::ID::ROLL,
                                  MidiControlId::BUTTON_ROLL)
    {
        setIcon(Icons::createDiceIcon());
        setLabel("Roll");
    }

    void mouseDown(const juce::MouseEvent& event) override {
        if (locked_) {
            // Click while locked: unlock and turn off
            locked_ = false;
            setParamOn(false);
            return;
        }

        // Start tracking for potential swipe-up
        dragStartY_ = event.getPosition().y;
        swipedUp_ = false;

        // Normal momentary behavior: turn on
        SequencerControlButton::mouseDown(event);
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        if (locked_) return;

        constexpr int swipeThreshold = 15;
        int deltaY = dragStartY_ - event.getPosition().y;
        if (!swipedUp_ && deltaY > swipeThreshold) {
            swipedUp_ = true;
            locked_ = true;
            setParamOn(true);
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent& event) override {
        if (locked_) {
            // Locked: keep param on, don't release
            return;
        }
        // Normal momentary release
        SequencerControlButton::mouseUp(event);
    }

protected:
    std::pair<juce::Colour, juce::Colour> getTargetColours() const override {
        using namespace SequencerButtonConstants;
        if (locked_) {
            return { juce::Colour(bgColorLocked), juce::Colours::white };
        }
        return SequencerControlButton::getTargetColours();
    }

private:
    bool locked_ = false;
    bool swipedUp_ = false;
    int dragStartY_ = 0;
};
