#pragma once

#include <juce_graphics/juce_graphics.h>

namespace Icons {

    /**
     * Creates the save (floppy disk) icon path
     * Original SVG viewBox: 0 0 13 13
     * Classic floppy disk save icon
     */
    inline juce::Path createSaveIcon() {
        juce::Path path;
        path.startNewSubPath (9.7f, 0.0f);
        path.cubicTo (9.9f, 0.0f, 10.1f, 0.1f, 10.3f, 0.3f);
        path.lineTo (12.7f, 2.7f);
        path.lineTo (12.7f, 2.7f);
        path.cubicTo (12.9f, 2.9f, 13.0f, 3.2f, 13.0f, 3.5f);
        path.lineTo (13.0f, 12.0f);
        path.cubicTo (13.0f, 12.2f, 12.9f, 12.5f, 12.7f, 12.7f);
        path.cubicTo (12.5f, 12.9f, 12.2f, 13.0f, 12.0f, 13.0f);
        path.lineTo (1.0f, 13.0f);
        path.cubicTo (0.8f, 13.0f, 0.5f, 12.9f, 0.3f, 12.7f);
        path.cubicTo (0.1f, 12.5f, 0.0f, 12.2f, 0.0f, 12.0f);
        path.lineTo (0.0f, 1.0f);
        path.cubicTo (0.0f, 0.8f, 0.1f, 0.5f, 0.3f, 0.3f);
        path.cubicTo (0.5f, 0.1f, 0.8f, 0.0f, 1.0f, 0.0f);
        path.lineTo (9.5f, 0.0f);
        path.lineTo (9.7f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath (1.0f, 1.0f);
        path.cubicTo (1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        path.lineTo (1.0f, 12.0f);
        path.cubicTo (1.0f, 12.0f, 1.0f, 12.0f, 1.0f, 12.0f);
        path.cubicTo (1.0f, 12.0f, 1.0f, 12.0f, 1.0f, 12.0f);
        path.lineTo (3.0f, 12.0f);
        path.lineTo (3.0f, 8.1f);
        path.cubicTo (3.0f, 7.8f, 3.1f, 7.5f, 3.3f, 7.3f);
        path.cubicTo (3.5f, 7.1f, 3.7f, 7.0f, 4.0f, 7.0f);
        path.lineTo (9.0f, 7.0f);
        path.cubicTo (9.3f, 7.0f, 9.5f, 7.1f, 9.7f, 7.3f);
        path.cubicTo (9.9f, 7.5f, 10.0f, 7.8f, 10.0f, 8.1f);
        path.lineTo (10.0f, 12.0f);
        path.lineTo (12.0f, 12.0f);
        path.cubicTo (12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f);
        path.cubicTo (12.0f, 12.0f, 12.0f, 12.0f, 12.0f, 12.0f);
        path.lineTo (12.0f, 3.5f);
        path.cubicTo (12.0f, 3.4f, 12.0f, 3.4f, 12.0f, 3.4f);
        path.lineTo (9.6f, 1.0f);
        path.lineTo (9.6f, 1.0f);
        path.lineTo (9.5f, 1.0f);
        path.lineTo (1.0f, 1.0f);
        path.cubicTo (1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        path.closeSubPath();
        path.startNewSubPath (4.0f, 8.0f);
        path.cubicTo (4.0f, 8.0f, 4.0f, 8.0f, 4.0f, 8.1f);
        path.lineTo (4.0f, 12.0f);
        path.lineTo (9.0f, 12.0f);
        path.lineTo (9.0f, 8.1f);
        path.cubicTo (9.0f, 8.0f, 9.0f, 8.0f, 9.0f, 8.0f);
        path.lineTo (4.0f, 8.0f);
        path.closeSubPath();
        path.startNewSubPath (8.5f, 2.0f);
        path.cubicTo (8.8f, 2.0f, 9.0f, 2.2f, 9.0f, 2.5f);
        path.cubicTo (9.0f, 2.8f, 8.8f, 3.0f, 8.5f, 3.0f);
        path.lineTo (4.5f, 3.0f);
        path.cubicTo (4.2f, 3.0f, 4.0f, 2.8f, 4.0f, 2.5f);
        path.cubicTo (4.0f, 2.2f, 4.2f, 2.0f, 4.5f, 2.0f);
        path.lineTo (8.5f, 2.0f);
        path.closeSubPath();
        return path;
    }
    /**
     * Creates an export/download icon (arrow pointing down into a tray)
     * ViewBox: 0 0 24 24
     */
    inline juce::Path createExportIcon() {
        juce::Path path;

        // Down arrow shaft
        path.startNewSubPath(12.0f, 3.0f);
        path.lineTo(12.0f, 15.0f);

        // Arrow head
        path.startNewSubPath(7.0f, 11.0f);
        path.lineTo(12.0f, 16.0f);
        path.lineTo(17.0f, 11.0f);

        // Tray (U shape)
        path.startNewSubPath(4.0f, 14.0f);
        path.lineTo(4.0f, 20.0f);
        path.lineTo(20.0f, 20.0f);
        path.lineTo(20.0f, 14.0f);

        return path;
    }
    /**
     * Creates a "compact view" icon — two horizontal bars suggesting collapsed rows
     * Used to toggle between full view and compact (single-row) view
     */
    inline juce::Path createCompactViewIcon() {
        juce::Path path;
        // Top bar
        path.addRoundedRectangle(0.0f, 0.0f, 20.0f, 4.0f, 1.5f);
        // Bottom bar
        path.addRoundedRectangle(0.0f, 8.0f, 20.0f, 4.0f, 1.5f);
        // Down arrow beneath bars
        path.startNewSubPath(5.0f, 15.0f);
        path.lineTo(10.0f, 20.0f);
        path.lineTo(15.0f, 15.0f);
        return path;
    }
    /**
     * Creates a "compact top" icon — two bars with an up arrow above
     * Same as compact view icon but rotated 180° (arrow points up)
     */
    inline juce::Path createCompactTopIcon() {
        juce::Path path;
        // Up arrow above bars
        path.startNewSubPath(5.0f, 5.0f);
        path.lineTo(10.0f, 0.0f);
        path.lineTo(15.0f, 5.0f);
        // Top bar
        path.addRoundedRectangle(0.0f, 8.0f, 20.0f, 4.0f, 1.5f);
        // Bottom bar
        path.addRoundedRectangle(0.0f, 16.0f, 20.0f, 4.0f, 1.5f);
        return path;
    }
    /**
     * Creates a "swap rows" icon — up/down arrows
     * Used to swap between top row and bottom row in compact view
     */
    inline juce::Path createSwapRowsIcon() {
        juce::Path path;
        // Up arrow
        path.startNewSubPath(5.0f, 8.0f);
        path.lineTo(10.0f, 2.0f);
        path.lineTo(15.0f, 8.0f);
        // Down arrow
        path.startNewSubPath(5.0f, 14.0f);
        path.lineTo(10.0f, 20.0f);
        path.lineTo(15.0f, 14.0f);
        return path;
    }

} // namespace Icons
