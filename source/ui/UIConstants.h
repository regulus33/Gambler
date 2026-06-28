#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace UIConstants {
    // Configurable step indicator colours (defaults: white)
    // Active step = playback position fill colour
    // Selected step = editing selection outline colour
    inline juce::Colour activeStepColour { 0xFFFFF5E1 };          // light cream
    inline juce::Colour selectedStepOutlineColour { 0xFFFFF5E1 }; // light cream

    // Selected-step border fade (seconds)
    constexpr float borderFadeInDuration = 0.08f;    // Fast fade in
    constexpr float borderFadeOutDuration = 0.18f;   // Slightly slower fade out

    // Legacy alias kept for drag-highlight usage
    static inline const juce::Colour stepIndicatorWhite = juce::Colours::white;

    // Placeholder steps = un-initialized / out-of-bounds steps that lie beyond
    // the current sequence (or subsequence) length. These are drawn with a
    // transparent fill and a light-grey dashed outline so they read as "not a
    // real step yet", distinct from rests (which keep a solid grey fill).
    inline juce::Colour placeholderOutlineColour { 0x80a0a0a0 }; // semi-transparent light grey

    // App background colour. Real placeholder steps fill transparent and reveal
    // this (it is the main screen's bg). Used to "erase" opaque content when an
    // overlay needs to preview a step as a placeholder — e.g. the length-drag
    // preview painted on top of the opaque step buttons.
    inline juce::Colour appBackgroundColour { 0xFF1E1E1E };

    /** Draws a dashed rounded-rectangle outline for a placeholder/out-of-bounds step. */
    inline void drawPlaceholderOutline (juce::Graphics& g,
                                        juce::Rectangle<float> bounds,
                                        float cornerRadius,
                                        juce::Colour colour = placeholderOutlineColour,
                                        float thickness = 1.0f) {
        juce::Path rect;
        rect.addRoundedRectangle (bounds.reduced (thickness * 0.5f), cornerRadius);

        const float dashes[] = { 3.0f, 3.0f };
        juce::Path dashed;
        juce::PathStrokeType (thickness).createDashedStroke (dashed, rect, dashes, 2);

        g.setColour (colour);
        g.fillPath (dashed);
    }
}
