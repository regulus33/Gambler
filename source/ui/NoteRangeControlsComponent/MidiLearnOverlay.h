#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * Transparent overlay that sits on top of a slider during MIDI learn mode.
 * Captures clicks so the slider doesn't receive them. The parent paints the
 * learn-mode visuals.
 */
class MidiLearnOverlay : public juce::Component {
public:
    std::function<void()> onMouseDown;

    void mouseDown(const juce::MouseEvent&) override {
        if (onMouseDown) onMouseDown();
    }

    void paint(juce::Graphics&) override {
        // Transparent; parent paints the overlay visuals.
    }
};
