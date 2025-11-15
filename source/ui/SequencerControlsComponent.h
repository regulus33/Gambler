#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Icon path generators for control buttons
 */
namespace ControlIcons {
    /**
     * Creates the dice/roll icon path
     * Original SVG viewBox: 0 0 22.3 22.8
     */
    inline juce::Path createRollIcon() {
        juce::Path path;
        path.startNewSubPath(18.6f, 0.0f);
        path.lineTo(3.7f, 0.0f);
        path.cubicTo(2.7f, 0.0f, 1.8f, 0.4f, 1.1f, 1.1f);
        path.cubicTo(0.4f, 1.8f, 0.0f, 2.8f, 0.0f, 3.8f);
        path.lineTo(0.0f, 19.0f);
        path.cubicTo(0.0f, 20.0f, 0.4f, 20.9f, 1.1f, 21.6f);
        path.cubicTo(1.8f, 22.4f, 2.7f, 22.8f, 3.7f, 22.8f);
        path.lineTo(18.6f, 22.8f);
        path.cubicTo(19.5f, 22.8f, 20.5f, 22.4f, 21.2f, 21.6f);
        path.cubicTo(21.9f, 20.9f, 22.3f, 20.0f, 22.3f, 19.0f);
        path.lineTo(22.3f, 3.8f);
        path.cubicTo(22.3f, 2.8f, 21.9f, 1.8f, 21.2f, 1.1f);
        path.cubicTo(20.5f, 0.4f, 19.5f, 0.0f, 18.6f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath(20.4f, 19.0f);
        path.cubicTo(20.4f, 19.5f, 20.2f, 19.9f, 19.9f, 20.3f);
        path.cubicTo(19.5f, 20.7f, 19.0f, 20.9f, 18.6f, 20.9f);
        path.lineTo(3.7f, 20.9f);
        path.cubicTo(3.2f, 20.9f, 2.7f, 20.7f, 2.4f, 20.3f);
        path.cubicTo(2.1f, 19.9f, 1.9f, 19.5f, 1.9f, 19.0f);
        path.lineTo(1.9f, 3.8f);
        path.cubicTo(1.9f, 3.3f, 2.1f, 2.8f, 2.4f, 2.5f);
        path.cubicTo(2.7f, 2.1f, 3.2f, 1.9f, 3.7f, 1.9f);
        path.lineTo(18.6f, 1.9f);
        path.cubicTo(19.0f, 1.9f, 19.5f, 2.1f, 19.9f, 2.5f);
        path.cubicTo(20.2f, 2.8f, 20.4f, 3.3f, 20.4f, 3.8f);
        path.lineTo(20.4f, 19.0f);
        path.closeSubPath();
        path.startNewSubPath(8.3f, 7.1f);
        path.cubicTo(8.3f, 7.4f, 8.3f, 7.7f, 8.1f, 7.9f);
        path.cubicTo(8.0f, 8.1f, 7.7f, 8.3f, 7.5f, 8.4f);
        path.cubicTo(7.2f, 8.5f, 7.0f, 8.6f, 6.7f, 8.5f);
        path.cubicTo(6.4f, 8.4f, 6.2f, 8.3f, 6.0f, 8.1f);
        path.cubicTo(5.8f, 7.9f, 5.6f, 7.7f, 5.6f, 7.4f);
        path.cubicTo(5.5f, 7.1f, 5.6f, 6.8f, 5.7f, 6.6f);
        path.cubicTo(5.8f, 6.3f, 6.0f, 6.1f, 6.2f, 5.9f);
        path.cubicTo(6.4f, 5.8f, 6.7f, 5.7f, 7.0f, 5.7f);
        path.cubicTo(7.3f, 5.7f, 7.7f, 5.8f, 7.9f, 6.1f);
        path.cubicTo(8.2f, 6.4f, 8.3f, 6.7f, 8.3f, 7.1f);
        path.closeSubPath();
        path.startNewSubPath(16.7f, 7.1f);
        path.cubicTo(16.7f, 7.4f, 16.6f, 7.7f, 16.5f, 7.9f);
        path.cubicTo(16.3f, 8.1f, 16.1f, 8.3f, 15.8f, 8.4f);
        path.cubicTo(15.6f, 8.5f, 15.3f, 8.6f, 15.0f, 8.5f);
        path.cubicTo(14.8f, 8.4f, 14.5f, 8.3f, 14.3f, 8.1f);
        path.cubicTo(14.1f, 7.9f, 14.0f, 7.7f, 13.9f, 7.4f);
        path.cubicTo(13.9f, 7.1f, 13.9f, 6.8f, 14.0f, 6.6f);
        path.cubicTo(14.1f, 6.3f, 14.3f, 6.1f, 14.5f, 5.9f);
        path.cubicTo(14.8f, 5.8f, 15.0f, 5.7f, 15.3f, 5.7f);
        path.cubicTo(15.7f, 5.7f, 16.0f, 5.8f, 16.3f, 6.1f);
        path.cubicTo(16.6f, 6.4f, 16.7f, 6.7f, 16.7f, 7.1f);
        path.closeSubPath();
        path.startNewSubPath(8.3f, 15.6f);
        path.cubicTo(8.3f, 15.9f, 8.3f, 16.2f, 8.1f, 16.4f);
        path.cubicTo(8.0f, 16.7f, 7.7f, 16.8f, 7.5f, 17.0f);
        path.cubicTo(7.2f, 17.1f, 7.0f, 17.1f, 6.7f, 17.0f);
        path.cubicTo(6.4f, 17.0f, 6.2f, 16.8f, 6.0f, 16.6f);
        path.cubicTo(5.8f, 16.4f, 5.6f, 16.2f, 5.6f, 15.9f);
        path.cubicTo(5.5f, 15.6f, 5.6f, 15.4f, 5.7f, 15.1f);
        path.cubicTo(5.8f, 14.8f, 6.0f, 14.6f, 6.2f, 14.5f);
        path.cubicTo(6.4f, 14.3f, 6.7f, 14.2f, 7.0f, 14.2f);
        path.cubicTo(7.3f, 14.2f, 7.7f, 14.4f, 7.9f, 14.6f);
        path.cubicTo(8.2f, 14.9f, 8.3f, 15.3f, 8.3f, 15.6f);
        path.closeSubPath();
        path.startNewSubPath(12.5f, 11.4f);
        path.cubicTo(12.5f, 11.7f, 12.4f, 11.9f, 12.3f, 12.2f);
        path.cubicTo(12.1f, 12.4f, 11.9f, 12.6f, 11.7f, 12.7f);
        path.cubicTo(11.4f, 12.8f, 11.1f, 12.8f, 10.9f, 12.8f);
        path.cubicTo(10.6f, 12.7f, 10.3f, 12.6f, 10.1f, 12.4f);
        path.cubicTo(10.0f, 12.2f, 9.8f, 11.9f, 9.8f, 11.7f);
        path.cubicTo(9.7f, 11.4f, 9.7f, 11.1f, 9.8f, 10.8f);
        path.cubicTo(10.0f, 10.6f, 10.1f, 10.3f, 10.4f, 10.2f);
        path.cubicTo(10.6f, 10.0f, 10.9f, 10.0f, 11.1f, 10.0f);
        path.cubicTo(11.5f, 10.0f, 11.9f, 10.1f, 12.1f, 10.4f);
        path.cubicTo(12.4f, 10.6f, 12.5f, 11.0f, 12.5f, 11.4f);
        path.closeSubPath();
        path.startNewSubPath(16.7f, 15.6f);
        path.cubicTo(16.7f, 15.9f, 16.6f, 16.2f, 16.5f, 16.4f);
        path.cubicTo(16.3f, 16.7f, 16.1f, 16.8f, 15.8f, 17.0f);
        path.cubicTo(15.6f, 17.1f, 15.3f, 17.1f, 15.0f, 17.0f);
        path.cubicTo(14.8f, 17.0f, 14.5f, 16.8f, 14.3f, 16.6f);
        path.cubicTo(14.1f, 16.4f, 14.0f, 16.2f, 13.9f, 15.9f);
        path.cubicTo(13.9f, 15.6f, 13.9f, 15.4f, 14.0f, 15.1f);
        path.cubicTo(14.1f, 14.8f, 14.3f, 14.6f, 14.5f, 14.5f);
        path.cubicTo(14.8f, 14.3f, 15.0f, 14.2f, 15.3f, 14.2f);
        path.cubicTo(15.7f, 14.2f, 16.0f, 14.4f, 16.3f, 14.6f);
        path.cubicTo(16.6f, 14.9f, 16.7f, 15.3f, 16.7f, 15.6f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the shift (up arrow) icon path
     * Original SVG viewBox: 0 0 14.3 14.6
     */
    inline juce::Path createShiftIcon() {
        juce::Path path;
        path.startNewSubPath(14.1f, 6.9f);
        path.lineTo(7.5f, 0.2f);
        path.cubicTo(7.5f, 0.1f, 7.4f, 0.1f, 7.4f, 0.0f);
        path.cubicTo(7.3f, 0.0f, 7.2f, 0.0f, 7.1f, 0.0f);
        path.cubicTo(7.1f, 0.0f, 7.0f, 0.0f, 6.9f, 0.0f);
        path.cubicTo(6.9f, 0.1f, 6.8f, 0.1f, 6.8f, 0.2f);
        path.lineTo(0.2f, 6.9f);
        path.cubicTo(0.1f, 7.0f, 0.0f, 7.1f, 0.0f, 7.2f);
        path.cubicTo(-0.0f, 7.3f, 0.0f, 7.4f, 0.0f, 7.5f);
        path.cubicTo(0.1f, 7.6f, 0.2f, 7.7f, 0.2f, 7.8f);
        path.cubicTo(0.3f, 7.8f, 0.4f, 7.9f, 0.6f, 7.9f);
        path.lineTo(3.3f, 7.9f);
        path.lineTo(3.3f, 11.8f);
        path.cubicTo(3.3f, 11.9f, 3.4f, 12.1f, 3.5f, 12.2f);
        path.cubicTo(3.6f, 12.3f, 3.7f, 12.4f, 3.8f, 12.4f);
        path.lineTo(10.4f, 12.4f);
        path.cubicTo(10.6f, 12.4f, 10.7f, 12.3f, 10.8f, 12.2f);
        path.cubicTo(10.9f, 12.1f, 11.0f, 11.9f, 11.0f, 11.8f);
        path.lineTo(11.0f, 7.9f);
        path.lineTo(13.7f, 7.9f);
        path.cubicTo(13.9f, 7.9f, 14.0f, 7.8f, 14.1f, 7.8f);
        path.cubicTo(14.1f, 7.7f, 14.2f, 7.6f, 14.3f, 7.5f);
        path.cubicTo(14.3f, 7.4f, 14.3f, 7.3f, 14.3f, 7.2f);
        path.cubicTo(14.3f, 7.1f, 14.2f, 7.0f, 14.1f, 6.9f);
        path.closeSubPath();
        path.startNewSubPath(10.4f, 6.7f);
        path.cubicTo(10.3f, 6.7f, 10.2f, 6.8f, 10.1f, 6.9f);
        path.cubicTo(10.0f, 7.0f, 9.9f, 7.2f, 9.9f, 7.3f);
        path.lineTo(9.9f, 11.2f);
        path.lineTo(4.4f, 11.2f);
        path.lineTo(4.4f, 7.3f);
        path.cubicTo(4.4f, 7.2f, 4.3f, 7.0f, 4.2f, 6.9f);
        path.cubicTo(4.1f, 6.8f, 4.0f, 6.7f, 3.8f, 6.7f);
        path.lineTo(1.9f, 6.7f);
        path.lineTo(7.1f, 1.4f);
        path.lineTo(12.4f, 6.7f);
        path.lineTo(10.4f, 6.7f);
        path.closeSubPath();
        path.startNewSubPath(11.0f, 14.0f);
        path.cubicTo(11.0f, 14.2f, 10.9f, 14.3f, 10.8f, 14.4f);
        path.cubicTo(10.7f, 14.5f, 10.6f, 14.6f, 10.4f, 14.6f);
        path.lineTo(3.8f, 14.6f);
        path.cubicTo(3.7f, 14.6f, 3.6f, 14.5f, 3.5f, 14.4f);
        path.cubicTo(3.4f, 14.3f, 3.3f, 14.2f, 3.3f, 14.0f);
        path.cubicTo(3.3f, 13.9f, 3.4f, 13.8f, 3.5f, 13.6f);
        path.cubicTo(3.6f, 13.5f, 3.7f, 13.5f, 3.8f, 13.5f);
        path.lineTo(10.4f, 13.5f);
        path.cubicTo(10.6f, 13.5f, 10.7f, 13.5f, 10.8f, 13.6f);
        path.cubicTo(10.9f, 13.8f, 11.0f, 13.9f, 11.0f, 14.0f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the index left (left arrow) icon path
     * Original SVG viewBox: 0 0 17.7 19.0
     */
    inline juce::Path createIndexLeftIcon() {
        juce::Path path;
        path.startNewSubPath(0.2f, 10.0f);
        path.lineTo(8.3f, 18.8f);
        path.cubicTo(8.4f, 18.9f, 8.6f, 19.0f, 8.7f, 19.0f);
        path.cubicTo(8.8f, 19.1f, 9.0f, 19.1f, 9.1f, 19.0f);
        path.cubicTo(9.2f, 18.9f, 9.3f, 18.9f, 9.4f, 18.7f);
        path.cubicTo(9.5f, 18.6f, 9.5f, 18.5f, 9.5f, 18.3f);
        path.lineTo(9.5f, 14.7f);
        path.lineTo(16.3f, 14.7f);
        path.cubicTo(16.7f, 14.7f, 17.0f, 14.5f, 17.3f, 14.2f);
        path.cubicTo(17.5f, 14.0f, 17.7f, 13.6f, 17.7f, 13.2f);
        path.lineTo(17.7f, 5.9f);
        path.cubicTo(17.7f, 5.5f, 17.5f, 5.1f, 17.3f, 4.8f);
        path.cubicTo(17.0f, 4.6f, 16.7f, 4.4f, 16.3f, 4.4f);
        path.lineTo(9.5f, 4.4f);
        path.lineTo(9.5f, 0.7f);
        path.cubicTo(9.5f, 0.6f, 9.5f, 0.4f, 9.4f, 0.3f);
        path.cubicTo(9.3f, 0.2f, 9.2f, 0.1f, 9.1f, 0.1f);
        path.cubicTo(9.0f, 0.0f, 8.8f, -0.0f, 8.7f, 0.0f);
        path.cubicTo(8.6f, 0.0f, 8.4f, 0.1f, 8.3f, 0.2f);
        path.lineTo(0.2f, 9.0f);
        path.cubicTo(0.1f, 9.1f, 0.1f, 9.2f, 0.1f, 9.2f);
        path.cubicTo(0.0f, 9.3f, -0.0f, 9.4f, -0.0f, 9.5f);
        path.cubicTo(-0.0f, 9.6f, 0.0f, 9.7f, 0.1f, 9.8f);
        path.cubicTo(0.1f, 9.9f, 0.1f, 10.0f, 0.2f, 10.0f);
        path.closeSubPath();
        path.startNewSubPath(8.1f, 2.5f);
        path.lineTo(8.1f, 5.1f);
        path.cubicTo(8.1f, 5.3f, 8.2f, 5.5f, 8.3f, 5.6f);
        path.cubicTo(8.5f, 5.8f, 8.6f, 5.9f, 8.8f, 5.9f);
        path.lineTo(16.3f, 5.9f);
        path.lineTo(16.3f, 13.2f);
        path.lineTo(8.8f, 13.2f);
        path.cubicTo(8.6f, 13.2f, 8.5f, 13.3f, 8.3f, 13.4f);
        path.cubicTo(8.2f, 13.5f, 8.1f, 13.7f, 8.1f, 13.9f);
        path.lineTo(8.1f, 16.6f);
        path.lineTo(1.6f, 9.5f);
        path.lineTo(8.1f, 2.5f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the index right (right arrow) icon path
     * Original SVG viewBox: 0 0 17.7 19.0
     */
    inline juce::Path createIndexRightIcon() {
        juce::Path path;
        path.startNewSubPath(17.5f, 9.0f);
        path.lineTo(9.4f, 0.2f);
        path.cubicTo(9.3f, 0.1f, 9.1f, 0.0f, 9.0f, 0.0f);
        path.cubicTo(8.9f, -0.0f, 8.7f, -0.0f, 8.6f, 0.1f);
        path.cubicTo(8.5f, 0.2f, 8.4f, 0.2f, 8.3f, 0.3f);
        path.cubicTo(8.2f, 0.4f, 8.2f, 0.6f, 8.2f, 0.7f);
        path.lineTo(8.2f, 4.4f);
        path.lineTo(1.4f, 4.4f);
        path.cubicTo(1.0f, 4.4f, 0.7f, 4.6f, 0.4f, 4.8f);
        path.cubicTo(0.2f, 5.1f, 0.0f, 5.5f, 0.0f, 5.9f);
        path.lineTo(0.0f, 13.2f);
        path.cubicTo(0.0f, 13.6f, 0.2f, 14.0f, 0.4f, 14.2f);
        path.cubicTo(0.7f, 14.5f, 1.0f, 14.7f, 1.4f, 14.7f);
        path.lineTo(8.2f, 14.7f);
        path.lineTo(8.2f, 18.3f);
        path.cubicTo(8.2f, 18.5f, 8.2f, 18.6f, 8.3f, 18.7f);
        path.cubicTo(8.4f, 18.9f, 8.5f, 18.9f, 8.6f, 19.0f);
        path.cubicTo(8.7f, 19.1f, 8.9f, 19.1f, 9.0f, 19.0f);
        path.cubicTo(9.1f, 19.0f, 9.3f, 18.9f, 9.4f, 18.8f);
        path.lineTo(17.5f, 10.0f);
        path.cubicTo(17.6f, 9.9f, 17.6f, 9.9f, 17.6f, 9.8f);
        path.cubicTo(17.7f, 9.7f, 17.7f, 9.6f, 17.7f, 9.5f);
        path.cubicTo(17.7f, 9.4f, 17.7f, 9.3f, 17.6f, 9.2f);
        path.cubicTo(17.6f, 9.1f, 17.6f, 9.1f, 17.5f, 9.0f);
        path.closeSubPath();
        path.startNewSubPath(9.6f, 16.6f);
        path.lineTo(9.6f, 13.9f);
        path.cubicTo(9.6f, 13.7f, 9.5f, 13.5f, 9.4f, 13.4f);
        path.cubicTo(9.2f, 13.3f, 9.1f, 13.2f, 8.9f, 13.2f);
        path.lineTo(1.4f, 13.2f);
        path.lineTo(1.4f, 5.9f);
        path.lineTo(8.9f, 5.9f);
        path.cubicTo(9.1f, 5.9f, 9.2f, 5.8f, 9.4f, 5.6f);
        path.cubicTo(9.5f, 5.5f, 9.6f, 5.3f, 9.6f, 5.1f);
        path.lineTo(9.6f, 2.5f);
        path.lineTo(16.1f, 9.5f);
        path.lineTo(9.6f, 16.6f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the length minus (horizontal line) icon path
     * Original SVG viewBox: 0 0 16.5 1.1
     */
    inline juce::Path createLengthMinusIcon() {
        juce::Path path;
        path.startNewSubPath(16.5f, 0.6f);
        path.cubicTo(16.5f, 0.7f, 16.4f, 0.9f, 16.3f, 1.0f);
        path.cubicTo(16.2f, 1.1f, 16.0f, 1.1f, 15.8f, 1.1f);
        path.lineTo(0.7f, 1.1f);
        path.cubicTo(0.5f, 1.1f, 0.3f, 1.1f, 0.2f, 1.0f);
        path.cubicTo(0.1f, 0.9f, 0.0f, 0.7f, 0.0f, 0.6f);
        path.cubicTo(0.0f, 0.4f, 0.1f, 0.3f, 0.2f, 0.2f);
        path.cubicTo(0.3f, 0.1f, 0.5f, 0.0f, 0.7f, 0.0f);
        path.lineTo(15.8f, 0.0f);
        path.cubicTo(16.0f, 0.0f, 16.2f, 0.1f, 16.3f, 0.2f);
        path.cubicTo(16.4f, 0.3f, 16.5f, 0.4f, 16.5f, 0.6f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the length plus (plus sign) icon path
     * Original SVG viewBox: 0 0 13.2 13.5
     */
    inline juce::Path createLengthPlusIcon() {
        juce::Path path;
        path.startNewSubPath(13.2f, 6.7f);
        path.cubicTo(13.2f, 6.9f, 13.1f, 7.0f, 13.0f, 7.1f);
        path.cubicTo(12.9f, 7.2f, 12.8f, 7.3f, 12.6f, 7.3f);
        path.lineTo(7.1f, 7.3f);
        path.lineTo(7.1f, 12.9f);
        path.cubicTo(7.1f, 13.1f, 7.1f, 13.2f, 7.0f, 13.3f);
        path.cubicTo(6.9f, 13.4f, 6.7f, 13.5f, 6.6f, 13.5f);
        path.cubicTo(6.5f, 13.5f, 6.3f, 13.4f, 6.2f, 13.3f);
        path.cubicTo(6.1f, 13.2f, 6.0f, 13.1f, 6.0f, 12.9f);
        path.lineTo(6.0f, 7.3f);
        path.lineTo(0.5f, 7.3f);
        path.cubicTo(0.4f, 7.3f, 0.3f, 7.2f, 0.2f, 7.1f);
        path.cubicTo(0.1f, 7.0f, 0.0f, 6.9f, 0.0f, 6.7f);
        path.cubicTo(0.0f, 6.6f, 0.1f, 6.4f, 0.2f, 6.3f);
        path.cubicTo(0.3f, 6.2f, 0.4f, 6.2f, 0.5f, 6.2f);
        path.lineTo(6.0f, 6.2f);
        path.lineTo(6.0f, 0.6f);
        path.cubicTo(6.0f, 0.4f, 6.1f, 0.3f, 6.2f, 0.2f);
        path.cubicTo(6.3f, 0.1f, 6.5f, 0.0f, 6.6f, 0.0f);
        path.cubicTo(6.7f, 0.0f, 6.9f, 0.1f, 7.0f, 0.2f);
        path.cubicTo(7.1f, 0.3f, 7.1f, 0.4f, 7.1f, 0.6f);
        path.lineTo(7.1f, 6.2f);
        path.lineTo(12.6f, 6.2f);
        path.cubicTo(12.8f, 6.2f, 12.9f, 6.2f, 13.0f, 6.3f);
        path.cubicTo(13.1f, 6.4f, 13.2f, 6.6f, 13.2f, 6.7f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the nudge left (circle with left arrow) icon path
     * Original SVG viewBox: 0 0 17.6 18.0
     */
    inline juce::Path createNudgeLeftIcon() {
        juce::Path path;
        path.startNewSubPath(8.8f, 0.0f);
        path.cubicTo(7.1f, 0.0f, 5.4f, 0.5f, 3.9f, 1.5f);
        path.cubicTo(2.5f, 2.5f, 1.3f, 3.9f, 0.7f, 5.5f);
        path.cubicTo(0.0f, 7.2f, -0.2f, 9.0f, 0.2f, 10.7f);
        path.cubicTo(0.5f, 12.5f, 1.3f, 14.1f, 2.6f, 15.3f);
        path.cubicTo(3.8f, 16.6f, 5.4f, 17.5f, 7.1f, 17.8f);
        path.cubicTo(8.8f, 18.2f, 10.6f, 18.0f, 12.2f, 17.3f);
        path.cubicTo(13.8f, 16.6f, 15.1f, 15.5f, 16.1f, 14.0f);
        path.cubicTo(17.1f, 12.5f, 17.6f, 10.8f, 17.6f, 9.0f);
        path.cubicTo(17.6f, 6.6f, 16.7f, 4.3f, 15.0f, 2.6f);
        path.cubicTo(13.4f, 1.0f, 11.1f, 0.0f, 8.8f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath(8.8f, 16.6f);
        path.cubicTo(7.3f, 16.6f, 5.9f, 16.1f, 4.7f, 15.3f);
        path.cubicTo(3.4f, 14.5f, 2.5f, 13.3f, 1.9f, 11.9f);
        path.cubicTo(1.4f, 10.5f, 1.2f, 9.0f, 1.5f, 7.5f);
        path.cubicTo(1.8f, 6.0f, 2.5f, 4.7f, 3.5f, 3.6f);
        path.cubicTo(4.6f, 2.5f, 5.9f, 1.8f, 7.3f, 1.5f);
        path.cubicTo(8.8f, 1.2f, 10.3f, 1.4f, 11.6f, 2.0f);
        path.cubicTo(13.0f, 2.5f, 14.2f, 3.5f, 15.0f, 4.8f);
        path.cubicTo(15.8f, 6.0f, 16.2f, 7.5f, 16.2f, 9.0f);
        path.cubicTo(16.2f, 11.0f, 15.5f, 12.9f, 14.1f, 14.4f);
        path.cubicTo(12.7f, 15.8f, 10.8f, 16.6f, 8.8f, 16.6f);
        path.closeSubPath();
        path.startNewSubPath(12.9f, 9.0f);
        path.cubicTo(12.9f, 9.2f, 12.8f, 9.3f, 12.7f, 9.5f);
        path.cubicTo(12.5f, 9.6f, 12.4f, 9.7f, 12.2f, 9.7f);
        path.lineTo(7.0f, 9.7f);
        path.lineTo(8.6f, 11.3f);
        path.cubicTo(8.7f, 11.3f, 8.7f, 11.4f, 8.7f, 11.5f);
        path.cubicTo(8.8f, 11.6f, 8.8f, 11.7f, 8.8f, 11.8f);
        path.cubicTo(8.8f, 11.8f, 8.8f, 11.9f, 8.7f, 12.0f);
        path.cubicTo(8.7f, 12.1f, 8.7f, 12.2f, 8.6f, 12.2f);
        path.cubicTo(8.5f, 12.3f, 8.5f, 12.4f, 8.4f, 12.4f);
        path.cubicTo(8.3f, 12.4f, 8.2f, 12.4f, 8.1f, 12.4f);
        path.cubicTo(8.0f, 12.4f, 7.9f, 12.4f, 7.9f, 12.4f);
        path.cubicTo(7.8f, 12.4f, 7.7f, 12.3f, 7.6f, 12.2f);
        path.lineTo(4.9f, 9.5f);
        path.cubicTo(4.9f, 9.4f, 4.8f, 9.3f, 4.8f, 9.3f);
        path.cubicTo(4.8f, 9.2f, 4.7f, 9.1f, 4.7f, 9.0f);
        path.cubicTo(4.7f, 8.9f, 4.8f, 8.8f, 4.8f, 8.7f);
        path.cubicTo(4.8f, 8.6f, 4.9f, 8.6f, 4.9f, 8.5f);
        path.lineTo(7.6f, 5.7f);
        path.cubicTo(7.8f, 5.6f, 7.9f, 5.5f, 8.1f, 5.5f);
        path.cubicTo(8.3f, 5.5f, 8.5f, 5.6f, 8.6f, 5.7f);
        path.cubicTo(8.7f, 5.9f, 8.8f, 6.0f, 8.8f, 6.2f);
        path.cubicTo(8.8f, 6.4f, 8.7f, 6.6f, 8.6f, 6.7f);
        path.lineTo(7.0f, 8.3f);
        path.lineTo(12.2f, 8.3f);
        path.cubicTo(12.4f, 8.3f, 12.5f, 8.4f, 12.7f, 8.5f);
        path.cubicTo(12.8f, 8.6f, 12.9f, 8.8f, 12.9f, 9.0f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the nudge right (circle with right arrow) icon path
     * Original SVG viewBox: 0 0 17.6 18.0
     */
    inline juce::Path createNudgeRightIcon() {
        juce::Path path;
        path.startNewSubPath(8.8f, 18.0f);
        path.cubicTo(10.5f, 18.0f, 12.2f, 17.4f, 13.7f, 16.5f);
        path.cubicTo(15.1f, 15.5f, 16.3f, 14.1f, 16.9f, 12.4f);
        path.cubicTo(17.6f, 10.8f, 17.8f, 9.0f, 17.4f, 7.2f);
        path.cubicTo(17.1f, 5.5f, 16.2f, 3.9f, 15.0f, 2.6f);
        path.cubicTo(13.8f, 1.4f, 12.2f, 0.5f, 10.5f, 0.2f);
        path.cubicTo(8.8f, -0.2f, 7.0f, 0.0f, 5.4f, 0.7f);
        path.cubicTo(3.8f, 1.4f, 2.4f, 2.5f, 1.5f, 4.0f);
        path.cubicTo(0.5f, 5.5f, 0.0f, 7.2f, 0.0f, 9.0f);
        path.cubicTo(0.0f, 11.4f, 0.9f, 13.7f, 2.6f, 15.3f);
        path.cubicTo(4.2f, 17.0f, 6.5f, 18.0f, 8.8f, 18.0f);
        path.closeSubPath();
        path.startNewSubPath(8.8f, 1.4f);
        path.cubicTo(10.3f, 1.4f, 11.7f, 1.8f, 12.9f, 2.7f);
        path.cubicTo(14.2f, 3.5f, 15.1f, 4.7f, 15.7f, 6.1f);
        path.cubicTo(16.2f, 7.5f, 16.4f, 9.0f, 16.1f, 10.5f);
        path.cubicTo(15.8f, 11.9f, 15.1f, 13.3f, 14.1f, 14.4f);
        path.cubicTo(13.0f, 15.4f, 11.7f, 16.2f, 10.2f, 16.4f);
        path.cubicTo(8.8f, 16.7f, 7.3f, 16.6f, 5.9f, 16.0f);
        path.cubicTo(4.6f, 15.4f, 3.4f, 14.5f, 2.6f, 13.2f);
        path.cubicTo(1.8f, 12.0f, 1.4f, 10.5f, 1.4f, 9.0f);
        path.cubicTo(1.4f, 7.0f, 2.1f, 5.0f, 3.5f, 3.6f);
        path.cubicTo(4.9f, 2.2f, 6.8f, 1.4f, 8.8f, 1.4f);
        path.closeSubPath();
        path.startNewSubPath(4.7f, 9.0f);
        path.cubicTo(4.7f, 8.8f, 4.8f, 8.6f, 4.9f, 8.5f);
        path.cubicTo(5.1f, 8.4f, 5.2f, 8.3f, 5.4f, 8.3f);
        path.lineTo(10.5f, 8.3f);
        path.lineTo(9.0f, 6.7f);
        path.cubicTo(8.9f, 6.6f, 8.9f, 6.6f, 8.8f, 6.5f);
        path.cubicTo(8.8f, 6.4f, 8.8f, 6.3f, 8.8f, 6.2f);
        path.cubicTo(8.8f, 6.1f, 8.8f, 6.0f, 8.8f, 6.0f);
        path.cubicTo(8.9f, 5.9f, 8.9f, 5.8f, 9.0f, 5.7f);
        path.cubicTo(9.1f, 5.7f, 9.1f, 5.6f, 9.2f, 5.6f);
        path.cubicTo(9.3f, 5.5f, 9.4f, 5.5f, 9.5f, 5.5f);
        path.cubicTo(9.6f, 5.5f, 9.7f, 5.5f, 9.7f, 5.6f);
        path.cubicTo(9.8f, 5.6f, 9.9f, 5.7f, 10.0f, 5.7f);
        path.lineTo(12.7f, 8.5f);
        path.cubicTo(12.7f, 8.6f, 12.8f, 8.6f, 12.8f, 8.7f);
        path.cubicTo(12.8f, 8.8f, 12.9f, 8.9f, 12.9f, 9.0f);
        path.cubicTo(12.9f, 9.1f, 12.8f, 9.2f, 12.8f, 9.3f);
        path.cubicTo(12.8f, 9.3f, 12.7f, 9.4f, 12.7f, 9.5f);
        path.lineTo(10.0f, 12.2f);
        path.cubicTo(9.8f, 12.4f, 9.7f, 12.4f, 9.5f, 12.4f);
        path.cubicTo(9.3f, 12.4f, 9.1f, 12.4f, 9.0f, 12.2f);
        path.cubicTo(8.9f, 12.1f, 8.8f, 11.9f, 8.8f, 11.8f);
        path.cubicTo(8.8f, 11.6f, 8.9f, 11.4f, 9.0f, 11.3f);
        path.lineTo(10.5f, 9.7f);
        path.lineTo(5.4f, 9.7f);
        path.cubicTo(5.2f, 9.7f, 5.1f, 9.6f, 4.9f, 9.5f);
        path.cubicTo(4.8f, 9.3f, 4.7f, 9.2f, 4.7f, 9.0f);
        path.closeSubPath();
        return path;
    }
}

/**
 * SequencerControlsComponent
 *
 * Displays 8 control buttons below the sequencer steps:
 * - Roll (toggle random mode)
 * - Shift (modifier key)
 * - Index <- (move window left)
 * - Index -> (move window right)
 * - Length + (increase sequence length)
 * - Length - (decrease sequence length)
 * - Nudge <- (shift notes left)
 * - Nudge -> (shift notes right)
 *
 * Each button is 84×84 pixels with 16px spacing, matching the SequencerStepsComponent above.
 */
class SequencerControlsComponent : public juce::Component {
public:
    // Compile-time constant for icon vertical offset (adjust as needed)
    static constexpr float ICON_VERTICAL_OFFSET = 12.0f;

    SequencerControlsComponent() {
        // Create 8 buttons
        for (int i = 0; i < 8; i++) {
            auto* button = new ControlButton();
            buttons.add(button);
            addAndMakeVisible(button);
        }
        
        // Set button icons and labels
        buttons[0]->setIcon(ControlIcons::createRollIcon());
        buttons[0]->setLabel("Roll");

        buttons[1]->setIcon(ControlIcons::createIndexLeftIcon());
        buttons[1]->setLabel("Index");

        buttons[2]->setIcon(ControlIcons::createIndexRightIcon());
        buttons[2]->setLabel("Index");

        buttons[3]->setIcon(ControlIcons::createLengthPlusIcon());
        buttons[3]->setLabel("Length");

        buttons[4]->setIcon(ControlIcons::createLengthMinusIcon());
        buttons[4]->setLabel("Length");

        buttons[5]->setIcon(ControlIcons::createNudgeLeftIcon());
        buttons[5]->setLabel("Nudge");

        buttons[6]->setIcon(ControlIcons::createNudgeRightIcon());
        buttons[6]->setLabel("Nudge");

        buttons[7]->setIcon(ControlIcons::createShiftIcon());
        buttons[7]->setLabel("Shift");

        // Set toggle state for Shift only
        buttons[7]->setToggleable(true);  // Shift
    }
    
    ~SequencerControlsComponent() override {
        buttons.clear(true);
    }
    
    void paint (juce::Graphics& g) override {
        // Background is transparent - buttons draw themselves
    }
    
    void resized() override {
        // Layout: 8 buttons in a row, each 84×84 with 16px gaps
        const float buttonSize = 84.0f;
        const float gap = 16.0f;
        
        for (int i = 0; i < buttons.size(); i++) {
            float x = i * (buttonSize + gap);
            buttons[i]->setBounds(static_cast<int>(x), 0, 
                                 static_cast<int>(buttonSize), 
                                 static_cast<int>(buttonSize));
        }
    }
    
    // Callbacks for button clicks
    std::function<void(int buttonIndex)> onButtonClicked;
    
    // Get button toggle state (for Roll and Shift)
    bool isButtonToggled(int index) const {
        if (index >= 0 && index < buttons.size()) {
            return buttons[index]->isToggled();
        }
        return false;
    }
    
    // Set button toggle state (for Roll and Shift)
    void setButtonToggled(int index, bool toggled) {
        if (index >= 0 && index < buttons.size()) {
            buttons[index]->setToggled(toggled);
        }
    }
    
private:
    /**
     * Individual control button
     * Matches the design: rounded rectangle with white border, grey background
     */
    class ControlButton : public juce::Component {
    public:
        ControlButton() = default;
        
        void paint (juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat();

            // Determine colors based on toggle state OR pressed state
            bool isActive = isToggled_ || isPressed_;
            juce::Colour bgColor = isActive ? juce::Colour(0xff4a4a4a) : juce::Colour(0xff2a2a2a);
            juce::Colour borderColor = juce::Colours::white;
            juce::Colour textColor = juce::Colours::white;
            
            const float cornerRadius = 4.0f;
            const float borderWidth = 2.0f;
            
            // Fill background
            g.setColour(bgColor);
            g.fillRoundedRectangle(bounds, cornerRadius);
            
            // Draw white border inside the bounds
            g.setColour(borderColor);
            float inset = borderWidth * 0.5f;
            juce::Rectangle<float> borderBounds = bounds.reduced(inset);
            float borderRadius = juce::jmax(0.0f, cornerRadius - inset);
            g.drawRoundedRectangle(borderBounds, borderRadius, borderWidth);
            
            // Draw icon and/or label text
            if (iconPath_.has_value()) {
                // Draw the icon path
                g.setColour(textColor);

                // Get the icon's bounding box
                auto iconBounds = iconPath_->getBounds();

                // Calculate scale to limit icon to 24px on longest side
                const float maxIconSize = 24.0f;
                float longestSide = juce::jmax(iconBounds.getWidth(), iconBounds.getHeight());
                float scale = maxIconSize / longestSide;

                // Calculate the space available for the icon (above the text)
                // Text is 12px from bottom, text height is ~12px, so reserve ~30px for text area
                float textAreaHeight = 30.0f;
                float iconAreaHeight = bounds.getHeight() - textAreaHeight;

                // Center the icon in the available space, then apply vertical offset
                float scaledWidth = iconBounds.getWidth() * scale;
                float scaledHeight = iconBounds.getHeight() * scale;
                float offsetX = (bounds.getWidth() - scaledWidth) * 0.5f;
                float offsetY = (iconAreaHeight - scaledHeight) * 0.5f + ICON_VERTICAL_OFFSET;

                // Apply transform and draw
                // Need to account for the icon's original position in its bounding box
                juce::AffineTransform transform = juce::AffineTransform::scale(scale)
                    .translated(offsetX - iconBounds.getX() * scale,
                               offsetY - iconBounds.getY() * scale);

                g.fillPath(*iconPath_, transform);
            }

            // Draw label text if present (below icon or centered if no icon)
            if (label_.isNotEmpty()) {
                g.setColour(textColor);
                juce::Font font(juce::FontOptions(12.0f));
                font.setBold(true);
                g.setFont(font);

                // Position text 12px from bottom
                float textHeight = 12.0f;
                float textY = bounds.getHeight() - 12.0f - textHeight;
                juce::Rectangle<float> textBounds(0, textY, bounds.getWidth(), textHeight);

                g.drawText(label_, textBounds.toNearestInt(), juce::Justification::centredBottom);
            }
        }
        
        void mouseDown (const juce::MouseEvent& /*event*/) override {
            if (isToggleable_) {
                setToggled(!isToggled_);
            } else {
                // For momentary buttons, show pressed state
                isPressed_ = true;
                repaint();
            }

            // Notify parent
            if (auto* parent = dynamic_cast<SequencerControlsComponent*>(getParentComponent())) {
                if (parent->onButtonClicked) {
                    // Find our index
                    int index = parent->buttons.indexOf(this);
                    parent->onButtonClicked(index);
                }
            }
        }

        void mouseUp (const juce::MouseEvent& /*event*/) override {
            // Clear pressed state for momentary buttons
            if (!isToggleable_ && isPressed_) {
                isPressed_ = false;
                repaint();
            }
        }
        
        void setLabel(const juce::String& label) {
            label_ = label;
            repaint();
        }

        void setIcon(const juce::Path& icon) {
            iconPath_ = icon;
            repaint();
        }

        void setToggleable(bool toggleable) {
            isToggleable_ = toggleable;
        }

        bool isToggled() const {
            return isToggled_;
        }

        void setToggled(bool toggled) {
            if (isToggled_ != toggled) {
                isToggled_ = toggled;
                repaint();
            }
        }

    private:
        juce::String label_;
        std::optional<juce::Path> iconPath_;
        bool isToggleable_ = false;
        bool isToggled_ = false;
        bool isPressed_ = false;  // Track pressed state for momentary buttons
    };
    
    juce::OwnedArray<ControlButton> buttons;
};

