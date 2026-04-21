#pragma once

#include <juce_graphics/juce_graphics.h>

/**
 * PianoKeyPaths - Factory methods for building piano-key juce::Path shapes.
 *
 * The paths are described in SVG coordinate space and are expected to be
 * transformed/scaled by the caller after construction.
 */
struct PianoKeyPaths {
    /**
     * Create white key path with notch on right side (C, F)
     */
    static juce::Path createWhiteKeyRightNotch (float xOffset) {
        juce::Path path;
        path.startNewSubPath (xOffset + 57.1f, 127.7f);
        path.cubicTo (xOffset + 57.1f, 124.9f, xOffset + 54.9f, 122.7f, xOffset + 52.1f, 122.7f);
        path.lineTo (xOffset + 46.3f, 122.7f);
        path.cubicTo (xOffset + 43.6f, 122.7f, xOffset + 41.3f, 120.4f, xOffset + 41.3f, 117.7f);
        path.lineTo (xOffset + 41.3f, 5.5f);
        path.cubicTo (xOffset + 41.3f, 2.7f, xOffset + 39.1f, 0.5f, xOffset + 36.3f, 0.5f);
        path.lineTo (xOffset + 5.5f, 0.5f);
        path.cubicTo (xOffset + 2.7f, 0.5f, xOffset + 0.5f, 2.7f, xOffset + 0.5f, 5.5f);
        path.lineTo (xOffset + 0.5f, 194.5f);
        path.cubicTo (xOffset + 0.5f, 197.3f, xOffset + 2.7f, 199.5f, xOffset + 5.5f, 199.5f);
        path.lineTo (xOffset + 52.1f, 199.5f);
        path.cubicTo (xOffset + 54.9f, 199.5f, xOffset + 57.1f, 197.3f, xOffset + 57.1f, 194.5f);
        path.lineTo (xOffset + 57.1f, 127.7f);
        path.closeSubPath();
        return path;
    }

    /**
     * Create black key path
     */
    static juce::Path createBlackKey (float xOffset) {
        juce::Path path;
        path.startNewSubPath (xOffset + 5.5f, 0.5f);
        path.cubicTo (xOffset + 2.7f, 0.5f, xOffset + 0.5f, 2.8f, xOffset + 0.5f, 5.5f);
        path.lineTo (xOffset + 1.1f, 117.5f);
        path.cubicTo (xOffset + 1.1f, 120.2f, xOffset + 3.3f, 122.4f, xOffset + 5.9f, 122.5f);
        path.lineTo (xOffset + 18.1f, 122.9f);
        path.lineTo (xOffset + 23.7f, 122.9f);
        path.cubicTo (xOffset + 23.7f, 122.9f, xOffset + 23.7f, 122.9f, xOffset + 23.7f, 122.8f);
        path.cubicTo (xOffset + 23.7f, 122.8f, xOffset + 23.7f, 122.8f, xOffset + 23.8f, 122.8f);
        path.lineTo (xOffset + 35.5f, 122.8f);
        path.cubicTo (xOffset + 38.3f, 122.8f, xOffset + 40.5f, 120.6f, xOffset + 40.5f, 117.8f);
        path.lineTo (xOffset + 40.5f, 5.5f);
        path.cubicTo (xOffset + 40.5f, 2.7f, xOffset + 38.3f, 0.5f, xOffset + 35.5f, 0.5f);
        path.lineTo (xOffset + 5.5f, 0.5f);
        path.closeSubPath();
        return path;
    }

    /**
     * Create white key path with notches on both sides (D, G, A)
     */
    static juce::Path createWhiteKeyBothNotches (float xOffset) {
        juce::Path path;
        path.startNewSubPath (xOffset + 60.7f, 127.7f);
        path.cubicTo (xOffset + 60.7f, 124.9f, xOffset + 58.4f, 122.7f, xOffset + 55.7f, 122.7f);
        path.lineTo (xOffset + 47.5f, 122.7f);
        path.cubicTo (xOffset + 44.7f, 122.7f, xOffset + 42.5f, 120.4f, xOffset + 42.5f, 117.7f);
        path.lineTo (xOffset + 42.5f, 5.5f);
        path.cubicTo (xOffset + 42.5f, 2.7f, xOffset + 40.3f, 0.5f, xOffset + 37.5f, 0.5f);
        path.lineTo (xOffset + 23.7f, 0.5f);
        path.cubicTo (xOffset + 20.9f, 0.5f, xOffset + 18.7f, 2.7f, xOffset + 18.7f, 5.5f);
        path.lineTo (xOffset + 18.7f, 117.7f);
        path.cubicTo (xOffset + 18.7f, 120.4f, xOffset + 16.5f, 122.7f, xOffset + 13.7f, 122.7f);
        path.lineTo (xOffset + 5.5f, 122.7f);
        path.cubicTo (xOffset + 2.7f, 122.7f, xOffset + 0.5f, 124.9f, xOffset + 0.5f, 127.7f);
        path.lineTo (xOffset + 0.5f, 194.5f);
        path.cubicTo (xOffset + 0.5f, 197.3f, xOffset + 2.7f, 199.5f, xOffset + 5.5f, 199.5f);
        path.lineTo (xOffset + 55.7f, 199.5f);
        path.cubicTo (xOffset + 58.4f, 199.5f, xOffset + 60.7f, 197.3f, xOffset + 60.7f, 194.5f);
        path.lineTo (xOffset + 60.7f, 127.7f);
        path.closeSubPath();
        return path;
    }

    /**
     * Create white key path with notch on left side (E, B)
     */
    static juce::Path createWhiteKeyLeftNotch (float xOffset) {
        juce::Path path;
        path.startNewSubPath (xOffset + 0.5f, 127.7f);
        path.cubicTo (xOffset + 0.5f, 124.9f, xOffset + 2.7f, 122.7f, xOffset + 5.5f, 122.7f);
        path.lineTo (xOffset + 11.3f, 122.7f);
        path.cubicTo (xOffset + 14.1f, 122.7f, xOffset + 16.3f, 120.4f, xOffset + 16.3f, 117.7f);
        path.lineTo (xOffset + 16.3f, 5.5f);
        path.cubicTo (xOffset + 16.3f, 2.7f, xOffset + 18.5f, 0.5f, xOffset + 21.3f, 0.5f);
        path.lineTo (xOffset + 52.1f, 0.5f);
        path.cubicTo (xOffset + 54.9f, 0.5f, xOffset + 57.1f, 2.7f, xOffset + 57.1f, 5.5f);
        path.lineTo (xOffset + 57.1f, 194.5f);
        path.cubicTo (xOffset + 57.1f, 197.3f, xOffset + 54.9f, 199.5f, xOffset + 52.1f, 199.5f);
        path.lineTo (xOffset + 5.5f, 199.5f);
        path.cubicTo (xOffset + 2.7f, 199.5f, xOffset + 0.5f, 197.3f, xOffset + 0.5f, 194.5f);
        path.lineTo (xOffset + 0.5f, 127.7f);
        path.closeSubPath();
        return path;
    }
};
