#pragma once

#include <juce_graphics/juce_graphics.h>

namespace Icons {

    /**
     * Create MIDI Learn icon
     * Design: Musical note with connection/link symbol
     * Size: ~18x18 coordinate space
     */
    inline juce::Path createMidiLearnIcon() {
        return createCrossHairIcon();
    }
    /**
     * Play Icon (triangle pointing right)
     * Design: Simple clean play button triangle
     * Size: ~9x9 coordinate space (half size for compact UI)
     */
    inline juce::Path createPlayIcon() {
        Path path;
        // Simple right-pointing triangle
        path.startNewSubPath (0.5f, 0.5f);
        path.lineTo (6.5f, 3.5f);
        path.lineTo (0.5f, 6.5f);
        path.closeSubPath();
        return path;
    }
    /**
     * Pause Icon (two vertical bars)
     * Design: Classic pause button with two parallel vertical bars
     * Size: ~9x9 coordinate space (half size for compact UI)
     */
    inline juce::Path createPauseIcon() {
        Path path;
        // Left bar
        path.startNewSubPath (1.0f, 0.5f);
        path.lineTo (1.0f, 6.5f);
        path.lineTo (2.5f, 6.5f);
        path.lineTo (2.5f, 0.5f);
        path.closeSubPath();
        // Right bar
        path.startNewSubPath (4.5f, 0.5f);
        path.lineTo (4.5f, 6.5f);
        path.lineTo (6.0f, 6.5f);
        path.lineTo (6.0f, 0.5f);
        path.closeSubPath();
        return path;
    }
    /**
     * Stop Icon (square)
     * Design: Classic stop button square with rounded corners
     * Size: ~9x9 coordinate space (half size for compact UI)
     */
    inline juce::Path createStopIcon() {
        Path path;
        // Simple rounded square
        path.startNewSubPath (5.5f, 0.5f);
        path.lineTo (1.5f, 0.5f);
        path.cubicTo (1.2f, 0.5f, 1.0f, 0.6f, 0.8f, 0.8f);
        path.cubicTo (0.6f, 1.0f, 0.5f, 1.2f, 0.5f, 1.5f);
        path.lineTo (0.5f, 5.5f);
        path.cubicTo (0.5f, 5.8f, 0.6f, 6.0f, 0.8f, 6.2f);
        path.cubicTo (1.0f, 6.4f, 1.2f, 6.5f, 1.5f, 6.5f);
        path.lineTo (5.5f, 6.5f);
        path.cubicTo (5.8f, 6.5f, 6.0f, 6.4f, 6.2f, 6.2f);
        path.cubicTo (6.4f, 6.0f, 6.5f, 5.8f, 6.5f, 5.5f);
        path.lineTo (6.5f, 1.5f);
        path.cubicTo (6.5f, 1.2f, 6.4f, 1.0f, 6.2f, 0.8f);
        path.cubicTo (6.0f, 0.6f, 5.8f, 0.5f, 5.5f, 0.5f);
        path.closeSubPath();
        return path;
    }

} // namespace Icons
