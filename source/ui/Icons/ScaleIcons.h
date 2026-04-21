#pragma once

#include <juce_graphics/juce_graphics.h>

namespace Icons {

    /**
     * Creates lock (closed) icon for scale latch
     * Simple padlock shape - closed/locked state
     * Original viewBox: 0 0 16 20
     */
    inline juce::Path createLockClosedIcon() {
        juce::Path path;
        path.startNewSubPath(19.2f, 7.9f);
        path.lineTo(15.8f, 7.9f);
        path.lineTo(15.8f, 5.2f);
        path.cubicTo(15.8f, 3.9f, 15.2f, 2.5f, 14.2f, 1.5f);
        path.cubicTo(13.2f, 0.6f, 11.9f, 0.0f, 10.5f, 0.0f);
        path.cubicTo(9.1f, 0.0f, 7.8f, 0.6f, 6.8f, 1.5f);
        path.cubicTo(5.8f, 2.5f, 5.2f, 3.9f, 5.2f, 5.2f);
        path.lineTo(5.2f, 7.9f);
        path.lineTo(1.8f, 7.9f);
        path.cubicTo(1.3f, 7.9f, 0.8f, 8.1f, 0.5f, 8.4f);
        path.cubicTo(0.2f, 8.7f, 0.0f, 9.2f, 0.0f, 9.6f);
        path.lineTo(0.0f, 21.9f);
        path.cubicTo(0.0f, 22.3f, 0.2f, 22.8f, 0.5f, 23.1f);
        path.cubicTo(0.8f, 23.4f, 1.3f, 23.6f, 1.8f, 23.6f);
        path.lineTo(19.2f, 23.6f);
        path.cubicTo(19.7f, 23.6f, 20.2f, 23.4f, 20.5f, 23.1f);
        path.cubicTo(20.8f, 22.8f, 21.0f, 22.3f, 21.0f, 21.9f);
        path.lineTo(21.0f, 9.6f);
        path.cubicTo(21.0f, 9.2f, 20.8f, 8.7f, 20.5f, 8.4f);
        path.cubicTo(20.2f, 8.1f, 19.7f, 7.9f, 19.2f, 7.9f);
        path.closeSubPath();
        path.startNewSubPath(7.0f, 5.2f);
        path.cubicTo(7.0f, 4.3f, 7.4f, 3.4f, 8.0f, 2.8f);
        path.cubicTo(8.7f, 2.1f, 9.6f, 1.8f, 10.5f, 1.8f);
        path.cubicTo(11.4f, 1.8f, 12.3f, 2.1f, 13.0f, 2.8f);
        path.cubicTo(13.6f, 3.4f, 14.0f, 4.3f, 14.0f, 5.2f);
        path.lineTo(14.0f, 7.9f);
        path.lineTo(7.0f, 7.9f);
        path.lineTo(7.0f, 5.2f);
        path.closeSubPath();
        path.startNewSubPath(19.2f, 21.9f);
        path.lineTo(1.8f, 21.9f);
        path.lineTo(1.8f, 9.6f);
        path.lineTo(19.2f, 9.6f);
        path.lineTo(19.2f, 21.9f);
        path.closeSubPath();
        path.startNewSubPath(11.8f, 15.8f);
        path.cubicTo(11.8f, 16.0f, 11.7f, 16.3f, 11.6f, 16.5f);
        path.cubicTo(11.4f, 16.7f, 11.2f, 16.9f, 11.0f, 17.0f);
        path.cubicTo(10.8f, 17.1f, 10.5f, 17.1f, 10.2f, 17.0f);
        path.cubicTo(10.0f, 17.0f, 9.8f, 16.9f, 9.6f, 16.7f);
        path.cubicTo(9.4f, 16.5f, 9.3f, 16.3f, 9.2f, 16.0f);
        path.cubicTo(9.2f, 15.8f, 9.2f, 15.5f, 9.3f, 15.2f);
        path.cubicTo(9.4f, 15.0f, 9.6f, 14.8f, 9.8f, 14.7f);
        path.cubicTo(10.0f, 14.5f, 10.2f, 14.4f, 10.5f, 14.4f);
        path.cubicTo(10.8f, 14.4f, 11.2f, 14.6f, 11.4f, 14.8f);
        path.cubicTo(11.7f, 15.1f, 11.8f, 15.4f, 11.8f, 15.8f);
        path.closeSubPath();
        return path;
    }
    /**
     * Creates unlock (open) icon for scale latch
     * Simple padlock shape - open/unlocked state
     * Original viewBox: 0 0 16 20
     */
    inline juce::Path createLockOpenIcon() {
        juce::Path path;
        path.startNewSubPath(19.2f, 7.9f);
        path.lineTo(7.0f, 7.9f);
        path.lineTo(7.0f, 5.2f);
        path.cubicTo(7.0f, 4.3f, 7.4f, 3.4f, 8.0f, 2.8f);
        path.cubicTo(8.7f, 2.1f, 9.6f, 1.8f, 10.5f, 1.8f);
        path.cubicTo(12.2f, 1.8f, 13.7f, 3.0f, 14.0f, 4.5f);
        path.cubicTo(14.1f, 4.8f, 14.2f, 5.0f, 14.4f, 5.1f);
        path.cubicTo(14.6f, 5.2f, 14.8f, 5.3f, 15.0f, 5.2f);
        path.cubicTo(15.3f, 5.2f, 15.5f, 5.0f, 15.6f, 4.9f);
        path.cubicTo(15.7f, 4.7f, 15.8f, 4.4f, 15.7f, 4.2f);
        path.cubicTo(15.2f, 1.8f, 13.0f, 0.0f, 10.5f, 0.0f);
        path.cubicTo(9.1f, 0.0f, 7.8f, 0.6f, 6.8f, 1.5f);
        path.cubicTo(5.8f, 2.5f, 5.3f, 3.9f, 5.2f, 5.2f);
        path.lineTo(5.2f, 7.9f);
        path.lineTo(1.8f, 7.9f);
        path.cubicTo(1.3f, 7.9f, 0.8f, 8.1f, 0.5f, 8.4f);
        path.cubicTo(0.2f, 8.7f, 0.0f, 9.2f, 0.0f, 9.6f);
        path.lineTo(0.0f, 21.9f);
        path.cubicTo(0.0f, 22.3f, 0.2f, 22.8f, 0.5f, 23.1f);
        path.cubicTo(0.8f, 23.4f, 1.3f, 23.6f, 1.8f, 23.6f);
        path.lineTo(19.2f, 23.6f);
        path.cubicTo(19.7f, 23.6f, 20.2f, 23.4f, 20.5f, 23.1f);
        path.cubicTo(20.8f, 22.8f, 21.0f, 22.3f, 21.0f, 21.9f);
        path.lineTo(21.0f, 9.6f);
        path.cubicTo(21.0f, 9.2f, 20.8f, 8.7f, 20.5f, 8.4f);
        path.cubicTo(20.2f, 8.1f, 19.7f, 7.9f, 19.2f, 7.9f);
        path.closeSubPath();
        path.startNewSubPath(19.2f, 21.9f);
        path.lineTo(1.8f, 21.9f);
        path.lineTo(1.8f, 9.6f);
        path.lineTo(19.2f, 9.6f);
        path.lineTo(19.2f, 21.9f);
        path.closeSubPath();
        path.startNewSubPath(11.8f, 15.8f);
        path.cubicTo(11.8f, 16.0f, 11.7f, 16.3f, 11.6f, 16.5f);
        path.cubicTo(11.4f, 16.7f, 11.2f, 16.9f, 11.0f, 17.0f);
        path.cubicTo(10.8f, 17.1f, 10.5f, 17.1f, 10.2f, 17.0f);
        path.cubicTo(10.0f, 17.0f, 9.8f, 16.9f, 9.6f, 16.7f);
        path.cubicTo(9.4f, 16.5f, 9.3f, 16.3f, 9.2f, 16.0f);
        path.cubicTo(9.2f, 15.8f, 9.2f, 15.5f, 9.3f, 15.2f);
        path.cubicTo(9.4f, 15.0f, 9.6f, 14.8f, 9.8f, 14.7f);
        path.cubicTo(10.0f, 14.5f, 10.2f, 14.4f, 10.5f, 14.4f);
        path.cubicTo(10.8f, 14.4f, 11.2f, 14.6f, 11.4f, 14.8f);
        path.cubicTo(11.7f, 15.1f, 11.8f, 15.4f, 11.8f, 15.8f);
        path.closeSubPath();

        return path;
    }
    /**
     * Creates a "bake scale" icon — musical note with a down arrow (stamp/apply).
     * Represents destructively applying scale quantization to steps.
     */
    inline juce::Path createBakeScaleIcon() {
        juce::Path path;
        // Musical note head (filled ellipse)
        path.addEllipse(1.0f, 13.0f, 7.0f, 5.0f);
        // Note stem
        path.addRoundedRectangle(6.0f, 2.0f, 2.0f, 13.0f, 0.5f);
        // Down arrow (stamp/bake)
        path.startNewSubPath(13.0f, 10.0f);
        path.lineTo(17.0f, 15.0f);
        path.lineTo(21.0f, 10.0f);
        path.closeSubPath();
        // Arrow stem (stops at triangle top edge to avoid even-odd overlap)
        path.addRoundedRectangle(16.0f, 3.0f, 2.0f, 7.0f, 0.5f);
        return path;
    }
    /**
     * Creates a "bake range" icon — vertical range bracket with a down arrow (stamp/apply).
     * Represents destructively applying note range to steps.
     */
    inline juce::Path createBakeRangeIcon() {
        juce::Path path;
        // Vertical bar (range indicator)
        path.addRoundedRectangle(1.0f, 2.0f, 3.0f, 16.0f, 1.0f);
        // Top cap
        path.addRoundedRectangle(0.0f, 1.0f, 7.0f, 2.0f, 0.5f);
        // Bottom cap
        path.addRoundedRectangle(0.0f, 17.0f, 7.0f, 2.0f, 0.5f);
        // Down arrow (stamp/bake)
        path.startNewSubPath(13.0f, 10.0f);
        path.lineTo(17.0f, 15.0f);
        path.lineTo(21.0f, 10.0f);
        path.closeSubPath();
        // Arrow stem (stops at triangle top edge to avoid even-odd overlap)
        path.addRoundedRectangle(16.0f, 3.0f, 2.0f, 7.0f, 0.5f);
        return path;
    }
    /**
     * Creates the divide/subdivide icon path.
     * 2x2 of small rounded squares with a gap between them, evoking "split
     * into quarters". Drawn into a 22x22 viewBox to match the other icons.
     */
    inline juce::Path createDivideIcon() {
        juce::Path path;
        constexpr float size = 9.0f;
        constexpr float gap  = 2.0f;
        constexpr float origin = 1.0f;           // leaves a 1px margin inside 22px
        constexpr float radius = 1.5f;
        const float second = origin + size + gap;

        path.addRoundedRectangle (origin, origin, size, size, radius);
        path.addRoundedRectangle (second, origin, size, size, radius);
        path.addRoundedRectangle (origin, second, size, size, radius);
        path.addRoundedRectangle (second, second, size, size, radius);
        return path;
    }

} // namespace Icons
