#pragma once

#include <juce_graphics/juce_graphics.h>

/**
 * KeyData - Data structure for a single piano key
 */
struct KeyData {
    juce::String name;
    juce::Colour color;
    bool isBlack;
    bool isToggled;
    juce::Path path;

    KeyData (const juce::String& n, const juce::Colour& c, bool black)
        : name (n), color (c), isBlack (black), isToggled (true) {}
};
