#pragma once

#include "../backend/MidiLearnRegistry.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MidiLearnOverlayPainter
 * 
 * Utility class for painting consistent MIDI learn overlays across all components.
 * Centralizes the visual style for MIDI learn mode to avoid duplication.
 * 
 * Usage:
 *   In your component's paint() method:
 *   MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, cornerRadius);
 */
class MidiLearnOverlayPainter {
public:
    // MIDI Learn overlay colors (centralized)
    static inline const juce::Colour overlayColor = juce::Colour(0xBB2255CC).darker ();      // Blue with ~73% opacity
    static inline const juce::Colour activeStrokeColor = juce::Colour(0xFFFFD700); // Yellow/gold stroke
    static inline const juce::Colour textColor = juce::Colour(0xFFFFD700);         // Yellow/gold text
    static constexpr float strokeWidth = 4.0f;
    static constexpr float textSize = 9.0f;

    /**
     * Paint MIDI learn overlay on a component
     *
     * @param g Graphics context
     * @param bounds Bounds to paint overlay in (usually getLocalBounds())
     * @param registry Pointer to MidiLearnRegistry (can be null)
     * @param controlId The MidiControlId for this component
     * @param cornerRadius Corner radius for rounded rectangle (0 for sharp corners)
     * @param textAreaFraction Fraction of bounds to use for text (0.0-1.0). If < 1.0, text is positioned in bottom portion. Default 1.0 = full bounds.
     */
    static void paint(juce::Graphics& g,
                     juce::Rectangle<float> bounds,
                     MidiLearnRegistry* registry,
                     MidiControlId controlId,
                     float cornerRadius = 0.0f,
                     float textAreaFraction = 1.0f) {
        // Early exit if not in learn mode
        if (!registry || !registry->isLearning()) return;
        if (controlId == MidiControlId::COUNT) return;

        bool isLearningThis = (registry->getLearningControl() == controlId);

        // Semi-transparent blue overlay
        g.setColour(overlayColor);
        if (cornerRadius > 0.0f) {
            g.fillRoundedRectangle(bounds, cornerRadius);
        } else {
            g.fillRect(bounds);
        }

        // Yellow/gold stroke if actively learning this control
        if (isLearningThis) {
            g.setColour(activeStrokeColor);
            if (cornerRadius > 0.0f) {
                g.drawRoundedRectangle(bounds.reduced(strokeWidth * 0.5f),
                                      cornerRadius,
                                      strokeWidth);
            } else {
                g.drawRect(bounds, strokeWidth);
            }
        }

        // Display MIDI mapping info (if mapped) in yellow
        if (const auto* midiId = registry->getMidiForControl(controlId)) {
            g.setColour(textColor);
            g.setFont(textSize);

            // Calculate text area based on textAreaFraction
            auto textBounds = bounds;
            if (textAreaFraction < 1.0f) {
                // Position text in the bottom portion (similar to piano key note names)
                float topTrim = bounds.getHeight() * (1.0f - textAreaFraction);
                textBounds = bounds.withTrimmedTop(topTrim);
            }

            g.drawText(midiId->toString(), textBounds.toNearestInt(), juce::Justification::centred);
        }
    }

    /**
     * Convenience overload that accepts integer bounds
     */
    static void paint(juce::Graphics& g,
                     juce::Rectangle<int> bounds,
                     MidiLearnRegistry* registry,
                     MidiControlId controlId,
                     float cornerRadius = 0.0f,
                     float textAreaFraction = 1.0f) {
        paint(g, bounds.toFloat(), registry, controlId, cornerRadius, textAreaFraction);
    }
};

