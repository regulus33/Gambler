#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Icons - Centralized icon system for the application
 * 
 * Provides static methods to create juce::Path icons with configurable size and color.
 * All icons are defined in their original SVG coordinate space and can be scaled/colored as needed.
 * 
 * Usage:
 *   auto path = Icons::createDiceIcon();
 *   auto transform = Icons::createTransform(path, targetBounds);
 *   g.setColour(Icons::defaultColor());
 *   g.fillPath(path, transform);
 * 
 * Or use the convenience method:
 *   Icons::drawIcon(g, Icons::createDiceIcon(), targetBounds, juce::Colours::white);
 */
class Icons {
public:
    // Default icon color
    static juce::Colour defaultColor() { return juce::Colours::white; }

    /**
     * Create a transform to fit an icon path into target bounds
     * Centers the icon and scales it to fit within the bounds
     */
    static juce::AffineTransform createTransform (const juce::Path& iconPath,
        const juce::Rectangle<float>& targetBounds,
        bool maintainAspectRatio = true) {
        auto pathBounds = iconPath.getBounds();

        float scaleX = targetBounds.getWidth() / pathBounds.getWidth();
        float scaleY = targetBounds.getHeight() / pathBounds.getHeight();
        float scale = maintainAspectRatio ? juce::jmin (scaleX, scaleY) : scaleX;

        // Calculate centered position
        float scaledWidth = pathBounds.getWidth() * scale;
        float scaledHeight = pathBounds.getHeight() * scale;
        float offsetX = targetBounds.getX() + (targetBounds.getWidth() - scaledWidth) * 0.5f;
        float offsetY = targetBounds.getY() + (targetBounds.getHeight() - scaledHeight) * 0.5f;

        return juce::AffineTransform::scale (scale)
            .translated (offsetX - pathBounds.getX() * scale,
                offsetY - pathBounds.getY() * scale);
    }

    /**
     * Convenience method to draw an icon with color
     */
    static void drawIcon (juce::Graphics& g,
        const juce::Path& iconPath,
        const juce::Rectangle<float>& targetBounds,
        const juce::Colour& color = juce::Colours::white,
        bool maintainAspectRatio = true) {
        auto transform = createTransform (iconPath, targetBounds, maintainAspectRatio);
        g.setColour (color);
        g.fillPath (iconPath, transform);
    }

    /**
     * Convenience method to draw an icon with stroke
     */
    static void strokeIcon (juce::Graphics& g,
        const juce::Path& iconPath,
        const juce::Rectangle<float>& targetBounds,
        const juce::Colour& color = juce::Colours::white,
        float strokeWidth = 1.5f,
        bool maintainAspectRatio = true) {
        auto transform = createTransform (iconPath, targetBounds, maintainAspectRatio);
        g.setColour (color);
        g.strokePath (iconPath, juce::PathStrokeType (strokeWidth), transform);
    }

    // ============================================================================
    // ICON CREATION METHODS
    // ============================================================================

    /**
     * Creates bar chart icon for MIDI data view
     * Original SVG viewBox: 0 0 23 21
     * Represents CC/modulation data visualization
     */
    static juce::Path createBarChartIcon() {
        juce::Path path;
        path.startNewSubPath (21.9f, 18.4f);
        path.lineTo (21.0f, 18.4f);
        path.lineTo (21.0f, 0.9f);
        path.cubicTo (21.0f, 0.6f, 20.9f, 0.4f, 20.7f, 0.3f);
        path.cubicTo (20.6f, 0.1f, 20.4f, 0.0f, 20.1f, 0.0f);
        path.lineTo (14.0f, 0.0f);
        path.cubicTo (13.8f, 0.0f, 13.5f, 0.1f, 13.4f, 0.3f);
        path.cubicTo (13.2f, 0.4f, 13.1f, 0.6f, 13.1f, 0.9f);
        path.lineTo (13.1f, 5.2f);
        path.lineTo (7.9f, 5.2f);
        path.cubicTo (7.6f, 5.2f, 7.4f, 5.3f, 7.3f, 5.5f);
        path.cubicTo (7.1f, 5.7f, 7.0f, 5.9f, 7.0f, 6.1f);
        path.lineTo (7.0f, 10.5f);
        path.lineTo (2.6f, 10.5f);
        path.cubicTo (2.4f, 10.5f, 2.2f, 10.6f, 2.0f, 10.8f);
        path.cubicTo (1.8f, 10.9f, 1.8f, 11.1f, 1.8f, 11.4f);
        path.lineTo (1.8f, 18.4f);
        path.lineTo (0.9f, 18.4f);
        path.cubicTo (0.6f, 18.4f, 0.4f, 18.5f, 0.3f, 18.6f);
        path.cubicTo (0.1f, 18.8f, 0.0f, 19.0f, 0.0f, 19.2f);
        path.cubicTo (0.0f, 19.5f, 0.1f, 19.7f, 0.3f, 19.9f);
        path.cubicTo (0.4f, 20.0f, 0.6f, 20.1f, 0.9f, 20.1f);
        path.lineTo (21.9f, 20.1f);
        path.cubicTo (22.1f, 20.1f, 22.3f, 20.0f, 22.5f, 19.9f);
        path.cubicTo (22.7f, 19.7f, 22.8f, 19.5f, 22.8f, 19.2f);
        path.cubicTo (22.8f, 19.0f, 22.7f, 18.8f, 22.5f, 18.6f);
        path.cubicTo (22.3f, 18.5f, 22.1f, 18.4f, 21.9f, 18.4f);
        path.closeSubPath();
        path.startNewSubPath (14.9f, 1.8f);
        path.lineTo (19.2f, 1.8f);
        path.lineTo (19.2f, 18.4f);
        path.lineTo (14.9f, 18.4f);
        path.lineTo (14.9f, 1.8f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 7.0f);
        path.lineTo (13.1f, 7.0f);
        path.lineTo (13.1f, 18.4f);
        path.lineTo (8.8f, 18.4f);
        path.lineTo (8.8f, 7.0f);
        path.closeSubPath();
        path.startNewSubPath (3.5f, 12.2f);
        path.lineTo (7.0f, 12.2f);
        path.lineTo (7.0f, 18.4f);
        path.lineTo (3.5f, 18.4f);
        path.lineTo (3.5f, 12.2f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the dice/roll icon path
     * Original SVG viewBox: 0 0 22.3 22.8
     */
    static juce::Path createDiceIcon() {
        juce::Path path;
        path.startNewSubPath (18.6f, 0.0f);
        path.lineTo (3.7f, 0.0f);
        path.cubicTo (2.7f, 0.0f, 1.8f, 0.4f, 1.1f, 1.1f);
        path.cubicTo (0.4f, 1.8f, 0.0f, 2.8f, 0.0f, 3.8f);
        path.lineTo (0.0f, 19.0f);
        path.cubicTo (0.0f, 20.0f, 0.4f, 20.9f, 1.1f, 21.6f);
        path.cubicTo (1.8f, 22.4f, 2.7f, 22.8f, 3.7f, 22.8f);
        path.lineTo (18.6f, 22.8f);
        path.cubicTo (19.5f, 22.8f, 20.5f, 22.4f, 21.2f, 21.6f);
        path.cubicTo (21.9f, 20.9f, 22.3f, 20.0f, 22.3f, 19.0f);
        path.lineTo (22.3f, 3.8f);
        path.cubicTo (22.3f, 2.8f, 21.9f, 1.8f, 21.2f, 1.1f);
        path.cubicTo (20.5f, 0.4f, 19.5f, 0.0f, 18.6f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath (20.4f, 19.0f);
        path.cubicTo (20.4f, 19.5f, 20.2f, 19.9f, 19.9f, 20.3f);
        path.cubicTo (19.5f, 20.7f, 19.0f, 20.9f, 18.6f, 20.9f);
        path.lineTo (3.7f, 20.9f);
        path.cubicTo (3.2f, 20.9f, 2.7f, 20.7f, 2.4f, 20.3f);
        path.cubicTo (2.1f, 19.9f, 1.9f, 19.5f, 1.9f, 19.0f);
        path.lineTo (1.9f, 3.8f);
        path.cubicTo (1.9f, 3.3f, 2.1f, 2.8f, 2.4f, 2.5f);
        path.cubicTo (2.7f, 2.1f, 3.2f, 1.9f, 3.7f, 1.9f);
        path.lineTo (18.6f, 1.9f);
        path.cubicTo (19.0f, 1.9f, 19.5f, 2.1f, 19.9f, 2.5f);
        path.cubicTo (20.2f, 2.8f, 20.4f, 3.3f, 20.4f, 3.8f);
        path.lineTo (20.4f, 19.0f);
        path.closeSubPath();
        path.startNewSubPath (8.3f, 7.1f);
        path.cubicTo (8.3f, 7.4f, 8.3f, 7.7f, 8.1f, 7.9f);
        path.cubicTo (8.0f, 8.1f, 7.7f, 8.3f, 7.5f, 8.4f);
        path.cubicTo (7.2f, 8.5f, 7.0f, 8.6f, 6.7f, 8.5f);
        path.cubicTo (6.4f, 8.4f, 6.2f, 8.3f, 6.0f, 8.1f);
        path.cubicTo (5.8f, 7.9f, 5.6f, 7.7f, 5.6f, 7.4f);
        path.cubicTo (5.5f, 7.1f, 5.6f, 6.8f, 5.7f, 6.6f);
        path.cubicTo (5.8f, 6.3f, 6.0f, 6.1f, 6.2f, 5.9f);
        path.cubicTo (6.4f, 5.8f, 6.7f, 5.7f, 7.0f, 5.7f);
        path.cubicTo (7.3f, 5.7f, 7.7f, 5.8f, 7.9f, 6.1f);
        path.cubicTo (8.2f, 6.4f, 8.3f, 6.7f, 8.3f, 7.1f);
        path.closeSubPath();
        path.startNewSubPath (16.7f, 7.1f);
        path.cubicTo (16.7f, 7.4f, 16.6f, 7.7f, 16.5f, 7.9f);
        path.cubicTo (16.3f, 8.1f, 16.1f, 8.3f, 15.8f, 8.4f);
        path.cubicTo (15.6f, 8.5f, 15.3f, 8.6f, 15.0f, 8.5f);
        path.cubicTo (14.8f, 8.4f, 14.5f, 8.3f, 14.3f, 8.1f);
        path.cubicTo (14.1f, 7.9f, 14.0f, 7.7f, 13.9f, 7.4f);
        path.cubicTo (13.9f, 7.1f, 13.9f, 6.8f, 14.0f, 6.6f);
        path.cubicTo (14.1f, 6.3f, 14.3f, 6.1f, 14.5f, 5.9f);
        path.cubicTo (14.8f, 5.8f, 15.0f, 5.7f, 15.3f, 5.7f);
        path.cubicTo (15.7f, 5.7f, 16.0f, 5.8f, 16.3f, 6.1f);
        path.cubicTo (16.6f, 6.4f, 16.7f, 6.7f, 16.7f, 7.1f);
        path.closeSubPath();
        path.startNewSubPath (8.3f, 15.6f);
        path.cubicTo (8.3f, 15.9f, 8.3f, 16.2f, 8.1f, 16.4f);
        path.cubicTo (8.0f, 16.7f, 7.7f, 16.8f, 7.5f, 17.0f);
        path.cubicTo (7.2f, 17.1f, 7.0f, 17.1f, 6.7f, 17.0f);
        path.cubicTo (6.4f, 17.0f, 6.2f, 16.8f, 6.0f, 16.6f);
        path.cubicTo (5.8f, 16.4f, 5.6f, 16.2f, 5.6f, 15.9f);
        path.cubicTo (5.5f, 15.6f, 5.6f, 15.4f, 5.7f, 15.1f);
        path.cubicTo (5.8f, 14.8f, 6.0f, 14.6f, 6.2f, 14.5f);
        path.cubicTo (6.4f, 14.3f, 6.7f, 14.2f, 7.0f, 14.2f);
        path.cubicTo (7.3f, 14.2f, 7.7f, 14.4f, 7.9f, 14.6f);
        path.cubicTo (8.2f, 14.9f, 8.3f, 15.3f, 8.3f, 15.6f);
        path.closeSubPath();
        path.startNewSubPath (12.5f, 11.4f);
        path.cubicTo (12.5f, 11.7f, 12.4f, 11.9f, 12.3f, 12.2f);
        path.cubicTo (12.1f, 12.4f, 11.9f, 12.6f, 11.7f, 12.7f);
        path.cubicTo (11.4f, 12.8f, 11.1f, 12.8f, 10.9f, 12.8f);
        path.cubicTo (10.6f, 12.7f, 10.3f, 12.6f, 10.1f, 12.4f);
        path.cubicTo (10.0f, 12.2f, 9.8f, 11.9f, 9.8f, 11.7f);
        path.cubicTo (9.7f, 11.4f, 9.7f, 11.1f, 9.8f, 10.8f);
        path.cubicTo (10.0f, 10.6f, 10.1f, 10.3f, 10.4f, 10.2f);
        path.cubicTo (10.6f, 10.0f, 10.9f, 10.0f, 11.1f, 10.0f);
        path.cubicTo (11.5f, 10.0f, 11.9f, 10.1f, 12.1f, 10.4f);
        path.cubicTo (12.4f, 10.6f, 12.5f, 11.0f, 12.5f, 11.4f);
        path.closeSubPath();
        path.startNewSubPath (16.7f, 15.6f);
        path.cubicTo (16.7f, 15.9f, 16.6f, 16.2f, 16.5f, 16.4f);
        path.cubicTo (16.3f, 16.7f, 16.1f, 16.8f, 15.8f, 17.0f);
        path.cubicTo (15.6f, 17.1f, 15.3f, 17.1f, 15.0f, 17.0f);
        path.cubicTo (14.8f, 17.0f, 14.5f, 16.8f, 14.3f, 16.6f);
        path.cubicTo (14.1f, 16.4f, 14.0f, 16.2f, 13.9f, 15.9f);
        path.cubicTo (13.9f, 15.6f, 13.9f, 15.4f, 14.0f, 15.1f);
        path.cubicTo (14.1f, 14.8f, 14.3f, 14.6f, 14.5f, 14.5f);
        path.cubicTo (14.8f, 14.3f, 15.0f, 14.2f, 15.3f, 14.2f);
        path.cubicTo (15.7f, 14.2f, 16.0f, 14.4f, 16.3f, 14.6f);
        path.cubicTo (16.6f, 14.9f, 16.7f, 15.3f, 16.7f, 15.6f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the shift (up arrow) icon path
     * Original SVG viewBox: 0 0 14.3 14.6
     */
    static juce::Path createShiftIcon() {
        juce::Path path;
        path.startNewSubPath (14.1f, 6.9f);
        path.lineTo (7.5f, 0.2f);
        path.cubicTo (7.5f, 0.1f, 7.4f, 0.1f, 7.4f, 0.0f);
        path.cubicTo (7.3f, 0.0f, 7.2f, 0.0f, 7.1f, 0.0f);
        path.cubicTo (7.1f, 0.0f, 7.0f, 0.0f, 6.9f, 0.0f);
        path.cubicTo (6.9f, 0.1f, 6.8f, 0.1f, 6.8f, 0.2f);
        path.lineTo (0.2f, 6.9f);
        path.cubicTo (0.1f, 7.0f, 0.0f, 7.1f, 0.0f, 7.2f);
        path.cubicTo (-0.0f, 7.3f, 0.0f, 7.4f, 0.0f, 7.5f);
        path.cubicTo (0.1f, 7.6f, 0.2f, 7.7f, 0.2f, 7.8f);
        path.cubicTo (0.3f, 7.8f, 0.4f, 7.9f, 0.6f, 7.9f);
        path.lineTo (3.3f, 7.9f);
        path.lineTo (3.3f, 11.8f);
        path.cubicTo (3.3f, 11.9f, 3.4f, 12.1f, 3.5f, 12.2f);
        path.cubicTo (3.6f, 12.3f, 3.7f, 12.4f, 3.8f, 12.4f);
        path.lineTo (10.4f, 12.4f);
        path.cubicTo (10.6f, 12.4f, 10.7f, 12.3f, 10.8f, 12.2f);
        path.cubicTo (10.9f, 12.1f, 11.0f, 11.9f, 11.0f, 11.8f);
        path.lineTo (11.0f, 7.9f);
        path.lineTo (13.7f, 7.9f);
        path.cubicTo (13.9f, 7.9f, 14.0f, 7.8f, 14.1f, 7.8f);
        path.cubicTo (14.1f, 7.7f, 14.2f, 7.6f, 14.3f, 7.5f);
        path.cubicTo (14.3f, 7.4f, 14.3f, 7.3f, 14.3f, 7.2f);
        path.cubicTo (14.3f, 7.1f, 14.2f, 7.0f, 14.1f, 6.9f);
        path.closeSubPath();
        path.startNewSubPath (10.4f, 6.7f);
        path.cubicTo (10.3f, 6.7f, 10.2f, 6.8f, 10.1f, 6.9f);
        path.cubicTo (10.0f, 7.0f, 9.9f, 7.2f, 9.9f, 7.3f);
        path.lineTo (9.9f, 11.2f);
        path.lineTo (4.4f, 11.2f);
        path.lineTo (4.4f, 7.3f);
        path.cubicTo (4.4f, 7.2f, 4.3f, 7.0f, 4.2f, 6.9f);
        path.cubicTo (4.1f, 6.8f, 4.0f, 6.7f, 3.8f, 6.7f);
        path.lineTo (1.9f, 6.7f);
        path.lineTo (7.1f, 1.4f);
        path.lineTo (12.4f, 6.7f);
        path.lineTo (10.4f, 6.7f);
        path.closeSubPath();
        path.startNewSubPath (11.0f, 14.0f);
        path.cubicTo (11.0f, 14.2f, 10.9f, 14.3f, 10.8f, 14.4f);
        path.cubicTo (10.7f, 14.5f, 10.6f, 14.6f, 10.4f, 14.6f);
        path.lineTo (3.8f, 14.6f);
        path.cubicTo (3.7f, 14.6f, 3.6f, 14.5f, 3.5f, 14.4f);
        path.cubicTo (3.4f, 14.3f, 3.3f, 14.2f, 3.3f, 14.0f);
        path.cubicTo (3.3f, 13.9f, 3.4f, 13.8f, 3.5f, 13.6f);
        path.cubicTo (3.6f, 13.5f, 3.7f, 13.5f, 3.8f, 13.5f);
        path.lineTo (10.4f, 13.5f);
        path.cubicTo (10.6f, 13.5f, 10.7f, 13.5f, 10.8f, 13.6f);
        path.cubicTo (10.9f, 13.8f, 11.0f, 13.9f, 11.0f, 14.0f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the index left (left arrow) icon path
     * Original SVG viewBox: 0 0 17.7 19.0
     */
    static juce::Path createIndexLeftIcon() {
        juce::Path path;
        path.startNewSubPath (0.2f, 10.0f);
        path.lineTo (8.3f, 18.8f);
        path.cubicTo (8.4f, 18.9f, 8.6f, 19.0f, 8.7f, 19.0f);
        path.cubicTo (8.8f, 19.1f, 9.0f, 19.1f, 9.1f, 19.0f);
        path.cubicTo (9.2f, 18.9f, 9.3f, 18.9f, 9.4f, 18.7f);
        path.cubicTo (9.5f, 18.6f, 9.5f, 18.5f, 9.5f, 18.3f);
        path.lineTo (9.5f, 14.7f);
        path.lineTo (16.3f, 14.7f);
        path.cubicTo (16.7f, 14.7f, 17.0f, 14.5f, 17.3f, 14.2f);
        path.cubicTo (17.5f, 14.0f, 17.7f, 13.6f, 17.7f, 13.2f);
        path.lineTo (17.7f, 5.9f);
        path.cubicTo (17.7f, 5.5f, 17.5f, 5.1f, 17.3f, 4.8f);
        path.cubicTo (17.0f, 4.6f, 16.7f, 4.4f, 16.3f, 4.4f);
        path.lineTo (9.5f, 4.4f);
        path.lineTo (9.5f, 0.7f);
        path.cubicTo (9.5f, 0.6f, 9.5f, 0.4f, 9.4f, 0.3f);
        path.cubicTo (9.3f, 0.2f, 9.2f, 0.1f, 9.1f, 0.1f);
        path.cubicTo (9.0f, 0.0f, 8.8f, -0.0f, 8.7f, 0.0f);
        path.cubicTo (8.6f, 0.0f, 8.4f, 0.1f, 8.3f, 0.2f);
        path.lineTo (0.2f, 9.0f);
        path.cubicTo (0.1f, 9.1f, 0.1f, 9.2f, 0.1f, 9.2f);
        path.cubicTo (0.0f, 9.3f, -0.0f, 9.4f, -0.0f, 9.5f);
        path.cubicTo (-0.0f, 9.6f, 0.0f, 9.7f, 0.1f, 9.8f);
        path.cubicTo (0.1f, 9.9f, 0.1f, 10.0f, 0.2f, 10.0f);
        path.closeSubPath();
        path.startNewSubPath (8.1f, 2.5f);
        path.lineTo (8.1f, 5.1f);
        path.cubicTo (8.1f, 5.3f, 8.2f, 5.5f, 8.3f, 5.6f);
        path.cubicTo (8.5f, 5.8f, 8.6f, 5.9f, 8.8f, 5.9f);
        path.lineTo (16.3f, 5.9f);
        path.lineTo (16.3f, 13.2f);
        path.lineTo (8.8f, 13.2f);
        path.cubicTo (8.6f, 13.2f, 8.5f, 13.3f, 8.3f, 13.4f);
        path.cubicTo (8.2f, 13.5f, 8.1f, 13.7f, 8.1f, 13.9f);
        path.lineTo (8.1f, 16.6f);
        path.lineTo (1.6f, 9.5f);
        path.lineTo (8.1f, 2.5f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the index right (right arrow) icon path
     * Original SVG viewBox: 0 0 17.7 19.0
     */
    static juce::Path createIndexRightIcon() {
        juce::Path path;
        path.startNewSubPath (17.5f, 9.0f);
        path.lineTo (9.4f, 0.2f);
        path.cubicTo (9.3f, 0.1f, 9.1f, 0.0f, 9.0f, 0.0f);
        path.cubicTo (8.9f, -0.0f, 8.7f, -0.0f, 8.6f, 0.1f);
        path.cubicTo (8.5f, 0.2f, 8.4f, 0.2f, 8.3f, 0.3f);
        path.cubicTo (8.2f, 0.4f, 8.2f, 0.6f, 8.2f, 0.7f);
        path.lineTo (8.2f, 4.4f);
        path.lineTo (1.4f, 4.4f);
        path.cubicTo (1.0f, 4.4f, 0.7f, 4.6f, 0.4f, 4.8f);
        path.cubicTo (0.2f, 5.1f, 0.0f, 5.5f, 0.0f, 5.9f);
        path.lineTo (0.0f, 13.2f);
        path.cubicTo (0.0f, 13.6f, 0.2f, 14.0f, 0.4f, 14.2f);
        path.cubicTo (0.7f, 14.5f, 1.0f, 14.7f, 1.4f, 14.7f);
        path.lineTo (8.2f, 14.7f);
        path.lineTo (8.2f, 18.3f);
        path.cubicTo (8.2f, 18.5f, 8.2f, 18.6f, 8.3f, 18.7f);
        path.cubicTo (8.4f, 18.9f, 8.5f, 18.9f, 8.6f, 19.0f);
        path.cubicTo (8.7f, 19.1f, 8.9f, 19.1f, 9.0f, 19.0f);
        path.cubicTo (9.1f, 19.0f, 9.3f, 18.9f, 9.4f, 18.8f);
        path.lineTo (17.5f, 10.0f);
        path.cubicTo (17.6f, 9.9f, 17.6f, 9.9f, 17.6f, 9.8f);
        path.cubicTo (17.7f, 9.7f, 17.7f, 9.6f, 17.7f, 9.5f);
        path.cubicTo (17.7f, 9.4f, 17.7f, 9.3f, 17.6f, 9.2f);
        path.cubicTo (17.6f, 9.1f, 17.6f, 9.1f, 17.5f, 9.0f);
        path.closeSubPath();
        path.startNewSubPath (9.6f, 16.6f);
        path.lineTo (9.6f, 13.9f);
        path.cubicTo (9.6f, 13.7f, 9.5f, 13.5f, 9.4f, 13.4f);
        path.cubicTo (9.2f, 13.3f, 9.1f, 13.2f, 8.9f, 13.2f);
        path.lineTo (1.4f, 13.2f);
        path.lineTo (1.4f, 5.9f);
        path.lineTo (8.9f, 5.9f);
        path.cubicTo (9.1f, 5.9f, 9.2f, 5.8f, 9.4f, 5.6f);
        path.cubicTo (9.5f, 5.5f, 9.6f, 5.3f, 9.6f, 5.1f);
        path.lineTo (9.6f, 2.5f);
        path.lineTo (16.1f, 9.5f);
        path.lineTo (9.6f, 16.6f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the length minus (horizontal line) icon path
     * Original SVG viewBox: 0 0 16.5 1.1
     */
    static juce::Path createLengthMinusIcon() {
        juce::Path path;
        path.startNewSubPath (16.5f, 0.6f);
        path.cubicTo (16.5f, 0.7f, 16.4f, 0.9f, 16.3f, 1.0f);
        path.cubicTo (16.2f, 1.1f, 16.0f, 1.1f, 15.8f, 1.1f);
        path.lineTo (0.7f, 1.1f);
        path.cubicTo (0.5f, 1.1f, 0.3f, 1.1f, 0.2f, 1.0f);
        path.cubicTo (0.1f, 0.9f, 0.0f, 0.7f, 0.0f, 0.6f);
        path.cubicTo (0.0f, 0.4f, 0.1f, 0.3f, 0.2f, 0.2f);
        path.cubicTo (0.3f, 0.1f, 0.5f, 0.0f, 0.7f, 0.0f);
        path.lineTo (15.8f, 0.0f);
        path.cubicTo (16.0f, 0.0f, 16.2f, 0.1f, 16.3f, 0.2f);
        path.cubicTo (16.4f, 0.3f, 16.5f, 0.4f, 16.5f, 0.6f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the length plus (plus sign) icon path
     * Original SVG viewBox: 0 0 13.2 13.5
     */
    static juce::Path createLengthPlusIcon() {
        juce::Path path;
        path.startNewSubPath (13.2f, 6.7f);
        path.cubicTo (13.2f, 6.9f, 13.1f, 7.0f, 13.0f, 7.1f);
        path.cubicTo (12.9f, 7.2f, 12.8f, 7.3f, 12.6f, 7.3f);
        path.lineTo (7.1f, 7.3f);
        path.lineTo (7.1f, 12.9f);
        path.cubicTo (7.1f, 13.1f, 7.1f, 13.2f, 7.0f, 13.3f);
        path.cubicTo (6.9f, 13.4f, 6.7f, 13.5f, 6.6f, 13.5f);
        path.cubicTo (6.5f, 13.5f, 6.3f, 13.4f, 6.2f, 13.3f);
        path.cubicTo (6.1f, 13.2f, 6.0f, 13.1f, 6.0f, 12.9f);
        path.lineTo (6.0f, 7.3f);
        path.lineTo (0.5f, 7.3f);
        path.cubicTo (0.4f, 7.3f, 0.3f, 7.2f, 0.2f, 7.1f);
        path.cubicTo (0.1f, 7.0f, 0.0f, 6.9f, 0.0f, 6.7f);
        path.cubicTo (0.0f, 6.6f, 0.1f, 6.4f, 0.2f, 6.3f);
        path.cubicTo (0.3f, 6.2f, 0.4f, 6.2f, 0.5f, 6.2f);
        path.lineTo (6.0f, 6.2f);
        path.lineTo (6.0f, 0.6f);
        path.cubicTo (6.0f, 0.4f, 6.1f, 0.3f, 6.2f, 0.2f);
        path.cubicTo (6.3f, 0.1f, 6.5f, 0.0f, 6.6f, 0.0f);
        path.cubicTo (6.7f, 0.0f, 6.9f, 0.1f, 7.0f, 0.2f);
        path.cubicTo (7.1f, 0.3f, 7.1f, 0.4f, 7.1f, 0.6f);
        path.lineTo (7.1f, 6.2f);
        path.lineTo (12.6f, 6.2f);
        path.cubicTo (12.8f, 6.2f, 12.9f, 6.2f, 13.0f, 6.3f);
        path.cubicTo (13.1f, 6.4f, 13.2f, 6.6f, 13.2f, 6.7f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the nudge left (circle with left arrow) icon path
     * Original SVG viewBox: 0 0 17.6 18.0
     */
    static juce::Path createNudgeLeftIcon() {
        juce::Path path;
        path.startNewSubPath (8.8f, 0.0f);
        path.cubicTo (7.1f, 0.0f, 5.4f, 0.5f, 3.9f, 1.5f);
        path.cubicTo (2.5f, 2.5f, 1.3f, 3.9f, 0.7f, 5.5f);
        path.cubicTo (0.0f, 7.2f, -0.2f, 9.0f, 0.2f, 10.7f);
        path.cubicTo (0.5f, 12.5f, 1.3f, 14.1f, 2.6f, 15.3f);
        path.cubicTo (3.8f, 16.6f, 5.4f, 17.5f, 7.1f, 17.8f);
        path.cubicTo (8.8f, 18.2f, 10.6f, 18.0f, 12.2f, 17.3f);
        path.cubicTo (13.8f, 16.6f, 15.1f, 15.5f, 16.1f, 14.0f);
        path.cubicTo (17.1f, 12.5f, 17.6f, 10.8f, 17.6f, 9.0f);
        path.cubicTo (17.6f, 6.6f, 16.7f, 4.3f, 15.0f, 2.6f);
        path.cubicTo (13.4f, 1.0f, 11.1f, 0.0f, 8.8f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 16.6f);
        path.cubicTo (7.3f, 16.6f, 5.9f, 16.1f, 4.7f, 15.3f);
        path.cubicTo (3.4f, 14.5f, 2.5f, 13.3f, 1.9f, 11.9f);
        path.cubicTo (1.4f, 10.5f, 1.2f, 9.0f, 1.5f, 7.5f);
        path.cubicTo (1.8f, 6.0f, 2.5f, 4.7f, 3.5f, 3.6f);
        path.cubicTo (4.6f, 2.5f, 5.9f, 1.8f, 7.3f, 1.5f);
        path.cubicTo (8.8f, 1.2f, 10.3f, 1.4f, 11.6f, 2.0f);
        path.cubicTo (13.0f, 2.5f, 14.2f, 3.5f, 15.0f, 4.8f);
        path.cubicTo (15.8f, 6.0f, 16.2f, 7.5f, 16.2f, 9.0f);
        path.cubicTo (16.2f, 11.0f, 15.5f, 12.9f, 14.1f, 14.4f);
        path.cubicTo (12.7f, 15.8f, 10.8f, 16.6f, 8.8f, 16.6f);
        path.closeSubPath();
        path.startNewSubPath (12.9f, 9.0f);
        path.cubicTo (12.9f, 9.2f, 12.8f, 9.3f, 12.7f, 9.5f);
        path.cubicTo (12.5f, 9.6f, 12.4f, 9.7f, 12.2f, 9.7f);
        path.lineTo (7.0f, 9.7f);
        path.lineTo (8.6f, 11.3f);
        path.cubicTo (8.7f, 11.3f, 8.7f, 11.4f, 8.7f, 11.5f);
        path.cubicTo (8.8f, 11.6f, 8.8f, 11.7f, 8.8f, 11.8f);
        path.cubicTo (8.8f, 11.8f, 8.8f, 11.9f, 8.7f, 12.0f);
        path.cubicTo (8.7f, 12.1f, 8.7f, 12.2f, 8.6f, 12.2f);
        path.cubicTo (8.5f, 12.3f, 8.5f, 12.4f, 8.4f, 12.4f);
        path.cubicTo (8.3f, 12.4f, 8.2f, 12.4f, 8.1f, 12.4f);
        path.cubicTo (8.0f, 12.4f, 7.9f, 12.4f, 7.9f, 12.4f);
        path.cubicTo (7.8f, 12.4f, 7.7f, 12.3f, 7.6f, 12.2f);
        path.lineTo (4.9f, 9.5f);
        path.cubicTo (4.9f, 9.4f, 4.8f, 9.3f, 4.8f, 9.3f);
        path.cubicTo (4.8f, 9.2f, 4.7f, 9.1f, 4.7f, 9.0f);
        path.cubicTo (4.7f, 8.9f, 4.8f, 8.8f, 4.8f, 8.7f);
        path.cubicTo (4.8f, 8.6f, 4.9f, 8.6f, 4.9f, 8.5f);
        path.lineTo (7.6f, 5.7f);
        path.cubicTo (7.8f, 5.6f, 7.9f, 5.5f, 8.1f, 5.5f);
        path.cubicTo (8.3f, 5.5f, 8.5f, 5.6f, 8.6f, 5.7f);
        path.cubicTo (8.7f, 5.9f, 8.8f, 6.0f, 8.8f, 6.2f);
        path.cubicTo (8.8f, 6.4f, 8.7f, 6.6f, 8.6f, 6.7f);
        path.lineTo (7.0f, 8.3f);
        path.lineTo (12.2f, 8.3f);
        path.cubicTo (12.4f, 8.3f, 12.5f, 8.4f, 12.7f, 8.5f);
        path.cubicTo (12.8f, 8.6f, 12.9f, 8.8f, 12.9f, 9.0f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the nudge right (circle with right arrow) icon path
     * Original SVG viewBox: 0 0 17.6 18.0
     */
    static juce::Path createNudgeRightIcon() {
        juce::Path path;
        path.startNewSubPath (8.8f, 18.0f);
        path.cubicTo (10.5f, 18.0f, 12.2f, 17.4f, 13.7f, 16.5f);
        path.cubicTo (15.1f, 15.5f, 16.3f, 14.1f, 16.9f, 12.4f);
        path.cubicTo (17.6f, 10.8f, 17.8f, 9.0f, 17.4f, 7.2f);
        path.cubicTo (17.1f, 5.5f, 16.2f, 3.9f, 15.0f, 2.6f);
        path.cubicTo (13.8f, 1.4f, 12.2f, 0.5f, 10.5f, 0.2f);
        path.cubicTo (8.8f, -0.2f, 7.0f, 0.0f, 5.4f, 0.7f);
        path.cubicTo (3.8f, 1.4f, 2.4f, 2.5f, 1.5f, 4.0f);
        path.cubicTo (0.5f, 5.5f, 0.0f, 7.2f, 0.0f, 9.0f);
        path.cubicTo (0.0f, 11.4f, 0.9f, 13.7f, 2.6f, 15.3f);
        path.cubicTo (4.2f, 17.0f, 6.5f, 18.0f, 8.8f, 18.0f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 1.4f);
        path.cubicTo (10.3f, 1.4f, 11.7f, 1.8f, 12.9f, 2.7f);
        path.cubicTo (14.2f, 3.5f, 15.1f, 4.7f, 15.7f, 6.1f);
        path.cubicTo (16.2f, 7.5f, 16.4f, 9.0f, 16.1f, 10.5f);
        path.cubicTo (15.8f, 11.9f, 15.1f, 13.3f, 14.1f, 14.4f);
        path.cubicTo (13.0f, 15.4f, 11.7f, 16.2f, 10.2f, 16.4f);
        path.cubicTo (8.8f, 16.7f, 7.3f, 16.6f, 5.9f, 16.0f);
        path.cubicTo (4.6f, 15.4f, 3.4f, 14.5f, 2.6f, 13.2f);
        path.cubicTo (1.8f, 12.0f, 1.4f, 10.5f, 1.4f, 9.0f);
        path.cubicTo (1.4f, 7.0f, 2.1f, 5.0f, 3.5f, 3.6f);
        path.cubicTo (4.9f, 2.2f, 6.8f, 1.4f, 8.8f, 1.4f);
        path.closeSubPath();
        path.startNewSubPath (4.7f, 9.0f);
        path.cubicTo (4.7f, 8.8f, 4.8f, 8.6f, 4.9f, 8.5f);
        path.cubicTo (5.1f, 8.4f, 5.2f, 8.3f, 5.4f, 8.3f);
        path.lineTo (10.5f, 8.3f);
        path.lineTo (9.0f, 6.7f);
        path.cubicTo (8.9f, 6.6f, 8.9f, 6.6f, 8.8f, 6.5f);
        path.cubicTo (8.8f, 6.4f, 8.8f, 6.3f, 8.8f, 6.2f);
        path.cubicTo (8.8f, 6.1f, 8.8f, 6.0f, 8.8f, 6.0f);
        path.cubicTo (8.9f, 5.9f, 8.9f, 5.8f, 9.0f, 5.7f);
        path.cubicTo (9.1f, 5.7f, 9.1f, 5.6f, 9.2f, 5.6f);
        path.cubicTo (9.3f, 5.5f, 9.4f, 5.5f, 9.5f, 5.5f);
        path.cubicTo (9.6f, 5.5f, 9.7f, 5.5f, 9.7f, 5.6f);
        path.cubicTo (9.8f, 5.6f, 9.9f, 5.7f, 10.0f, 5.7f);
        path.lineTo (12.7f, 8.5f);
        path.cubicTo (12.7f, 8.6f, 12.8f, 8.6f, 12.8f, 8.7f);
        path.cubicTo (12.8f, 8.8f, 12.9f, 8.9f, 12.9f, 9.0f);
        path.cubicTo (12.9f, 9.1f, 12.8f, 9.2f, 12.8f, 9.3f);
        path.cubicTo (12.8f, 9.3f, 12.7f, 9.4f, 12.7f, 9.5f);
        path.lineTo (10.0f, 12.2f);
        path.cubicTo (9.8f, 12.4f, 9.7f, 12.4f, 9.5f, 12.4f);
        path.cubicTo (9.3f, 12.4f, 9.1f, 12.4f, 9.0f, 12.2f);
        path.cubicTo (8.9f, 12.1f, 8.8f, 11.9f, 8.8f, 11.8f);
        path.cubicTo (8.8f, 11.6f, 8.9f, 11.4f, 9.0f, 11.3f);
        path.lineTo (10.5f, 9.7f);
        path.lineTo (5.4f, 9.7f);
        path.cubicTo (5.2f, 9.7f, 5.1f, 9.6f, 4.9f, 9.5f);
        path.cubicTo (4.8f, 9.3f, 4.7f, 9.2f, 4.7f, 9.0f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the settings (gear/cog) icon path
     * Original SVG viewBox: 0 0 24 24 (coords 2-22)
     * Note: This icon is typically drawn with stroke, not fill
     */
    static juce::Path createSettingsIcon() {
        juce::Path path;

        // Cog/gear icon - outer gear shape
        path.startNewSubPath (9.7f, 4.1f);
        path.cubicTo (9.7f, 3.6f, 10.0f, 3.0f, 10.4f, 2.6f);
        path.cubicTo (10.9f, 2.2f, 11.4f, 2.0f, 12.0f, 2.0f);
        path.cubicTo (12.6f, 2.0f, 13.1f, 2.2f, 13.6f, 2.6f);
        path.cubicTo (14.0f, 3.0f, 14.3f, 3.6f, 14.3f, 4.1f);
        path.cubicTo (14.4f, 4.5f, 14.5f, 4.9f, 14.7f, 5.2f);
        path.cubicTo (14.9f, 5.5f, 15.2f, 5.8f, 15.5f, 6.0f);
        path.cubicTo (15.8f, 6.1f, 16.2f, 6.3f, 16.6f, 6.3f);
        path.cubicTo (16.9f, 6.3f, 17.3f, 6.2f, 17.6f, 6.1f);
        path.cubicTo (18.2f, 5.8f, 18.8f, 5.8f, 19.3f, 6.0f);
        path.cubicTo (19.9f, 6.1f, 20.4f, 6.5f, 20.6f, 7.0f);
        path.cubicTo (20.9f, 7.5f, 21.0f, 8.1f, 20.9f, 8.7f);
        path.cubicTo (20.8f, 9.2f, 20.5f, 9.7f, 20.0f, 10.1f);
        path.cubicTo (19.7f, 10.3f, 19.4f, 10.6f, 19.2f, 10.9f);
        path.cubicTo (19.0f, 11.2f, 18.9f, 11.6f, 18.9f, 12.0f);
        path.cubicTo (18.9f, 12.4f, 19.0f, 12.8f, 19.2f, 13.1f);
        path.cubicTo (19.4f, 13.4f, 19.7f, 13.7f, 20.0f, 13.9f);
        path.cubicTo (20.5f, 14.3f, 20.8f, 14.8f, 20.9f, 15.3f);
        path.cubicTo (21.0f, 15.9f, 20.9f, 16.5f, 20.6f, 17.0f);
        path.cubicTo (20.4f, 17.5f, 19.9f, 17.9f, 19.3f, 18.0f);
        path.cubicTo (18.8f, 18.2f, 18.2f, 18.2f, 17.6f, 17.9f);
        path.cubicTo (17.3f, 17.8f, 16.9f, 17.7f, 16.6f, 17.7f);
        path.cubicTo (16.2f, 17.7f, 15.8f, 17.9f, 15.5f, 18.0f);
        path.cubicTo (15.2f, 18.2f, 14.9f, 18.5f, 14.7f, 18.8f);
        path.cubicTo (14.5f, 19.1f, 14.4f, 19.5f, 14.3f, 19.9f);
        path.cubicTo (14.3f, 20.4f, 14.0f, 21.0f, 13.6f, 21.4f);
        path.cubicTo (13.1f, 21.8f, 12.6f, 22.0f, 12.0f, 22.0f);
        path.cubicTo (11.4f, 22.0f, 10.9f, 21.8f, 10.4f, 21.4f);
        path.cubicTo (10.0f, 21.0f, 9.7f, 20.4f, 9.7f, 19.9f);
        path.cubicTo (9.6f, 19.5f, 9.5f, 19.1f, 9.3f, 18.8f);
        path.cubicTo (9.1f, 18.5f, 8.8f, 18.2f, 8.5f, 18.0f);
        path.cubicTo (8.2f, 17.9f, 7.8f, 17.7f, 7.4f, 17.7f);
        path.cubicTo (7.1f, 17.7f, 6.7f, 17.8f, 6.4f, 17.9f);
        path.cubicTo (5.8f, 18.2f, 5.2f, 18.2f, 4.7f, 18.0f);
        path.cubicTo (4.1f, 17.9f, 3.6f, 17.5f, 3.4f, 17.0f);
        path.cubicTo (3.1f, 16.5f, 3.0f, 15.9f, 3.1f, 15.3f);
        path.cubicTo (3.2f, 14.8f, 3.5f, 14.3f, 4.0f, 13.9f);
        path.cubicTo (4.3f, 13.7f, 4.6f, 13.4f, 4.8f, 13.1f);
        path.cubicTo (5.0f, 12.8f, 5.1f, 12.4f, 5.1f, 12.0f);
        path.cubicTo (5.1f, 11.6f, 5.0f, 11.2f, 4.8f, 10.9f);
        path.cubicTo (4.6f, 10.6f, 4.3f, 10.3f, 4.0f, 10.1f);
        path.cubicTo (3.5f, 9.7f, 3.2f, 9.2f, 3.1f, 8.7f);
        path.cubicTo (3.0f, 8.1f, 3.1f, 7.5f, 3.4f, 7.0f);
        path.cubicTo (3.6f, 6.5f, 4.1f, 6.1f, 4.7f, 6.0f);
        path.cubicTo (5.2f, 5.8f, 5.8f, 5.8f, 6.4f, 6.1f);
        path.cubicTo (6.7f, 6.2f, 7.1f, 6.3f, 7.4f, 6.3f);
        path.cubicTo (7.8f, 6.3f, 8.2f, 6.1f, 8.5f, 6.0f);
        path.cubicTo (8.8f, 5.8f, 9.1f, 5.5f, 9.3f, 5.2f);
        path.cubicTo (9.5f, 4.9f, 9.6f, 4.5f, 9.7f, 4.1f);

        // Center circle
        path.startNewSubPath (12.0f, 15.0f);
        path.cubicTo (13.7f, 15.0f, 15.0f, 13.7f, 15.0f, 12.0f);
        path.cubicTo (15.0f, 10.3f, 13.7f, 9.0f, 12.0f, 9.0f);
        path.cubicTo (10.3f, 9.0f, 9.0f, 10.3f, 9.0f, 12.0f);
        path.cubicTo (9.0f, 13.7f, 10.3f, 15.0f, 12.0f, 15.0f);
        path.closeSubPath();

        return path;
    }

    /**
     * Creates the keyboard icon path
     * Original SVG viewBox: 0 0 21 21
     * Represents a piano keyboard with white and black keys
     */
    static juce::Path createKeyboardIcon() {
        Path path;
        path.startNewSubPath (19.2f, 0.0f);
        path.lineTo (1.8f, 0.0f);
        path.cubicTo (1.3f, 0.0f, 0.8f, 0.2f, 0.5f, 0.5f);
        path.cubicTo (0.2f, 0.8f, 0.0f, 1.3f, 0.0f, 1.8f);
        path.lineTo (0.0f, 19.2f);
        path.cubicTo (0.0f, 19.7f, 0.2f, 20.2f, 0.5f, 20.5f);
        path.cubicTo (0.8f, 20.8f, 1.3f, 21.0f, 1.8f, 21.0f);
        path.lineTo (19.2f, 21.0f);
        path.cubicTo (19.7f, 21.0f, 20.2f, 20.8f, 20.5f, 20.5f);
        path.cubicTo (20.8f, 20.2f, 21.0f, 19.7f, 21.0f, 19.2f);
        path.lineTo (21.0f, 1.8f);
        path.cubicTo (21.0f, 1.3f, 20.8f, 0.8f, 20.5f, 0.5f);
        path.cubicTo (20.2f, 0.2f, 19.7f, 0.0f, 19.2f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath (5.2f, 1.8f);
        path.lineTo (7.9f, 1.8f);
        path.lineTo (7.9f, 11.4f);
        path.lineTo (5.2f, 11.4f);
        path.lineTo (5.2f, 1.8f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 13.1f);
        path.cubicTo (9.0f, 13.1f, 9.2f, 13.0f, 9.4f, 12.9f);
        path.cubicTo (9.5f, 12.7f, 9.6f, 12.5f, 9.6f, 12.2f);
        path.lineTo (9.6f, 1.8f);
        path.lineTo (11.4f, 1.8f);
        path.lineTo (11.4f, 12.2f);
        path.cubicTo (11.4f, 12.5f, 11.5f, 12.7f, 11.6f, 12.9f);
        path.cubicTo (11.8f, 13.0f, 12.0f, 13.1f, 12.2f, 13.1f);
        path.lineTo (13.1f, 13.1f);
        path.lineTo (13.1f, 19.2f);
        path.lineTo (7.9f, 19.2f);
        path.lineTo (7.9f, 13.1f);
        path.lineTo (8.8f, 13.1f);
        path.closeSubPath();
        path.startNewSubPath (13.1f, 11.4f);
        path.lineTo (13.1f, 1.8f);
        path.lineTo (15.8f, 1.8f);
        path.lineTo (15.8f, 11.4f);
        path.lineTo (13.1f, 11.4f);
        path.closeSubPath();
        path.startNewSubPath (1.8f, 1.8f);
        path.lineTo (3.5f, 1.8f);
        path.lineTo (3.5f, 12.2f);
        path.cubicTo (3.5f, 12.5f, 3.6f, 12.7f, 3.8f, 12.9f);
        path.cubicTo (3.9f, 13.0f, 4.1f, 13.1f, 4.4f, 13.1f);
        path.lineTo (6.1f, 13.1f);
        path.lineTo (6.1f, 19.2f);
        path.lineTo (1.8f, 19.2f);
        path.lineTo (1.8f, 1.8f);
        path.closeSubPath();
        path.startNewSubPath (19.2f, 19.2f);
        path.lineTo (14.9f, 19.2f);
        path.lineTo (14.9f, 13.1f);
        path.lineTo (16.6f, 13.1f);
        path.cubicTo (16.9f, 13.1f, 17.1f, 13.0f, 17.2f, 12.9f);
        path.cubicTo (17.4f, 12.7f, 17.5f, 12.5f, 17.5f, 12.2f);
        path.lineTo (17.5f, 1.8f);
        path.lineTo (19.2f, 1.8f);
        path.lineTo (19.2f, 19.2f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the save (floppy disk) icon path
     * Original SVG viewBox: 0 0 13 13
     * Classic floppy disk save icon
     */
    static juce::Path createSaveIcon() {
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
     * Creates the close (X) icon path
     * Original SVG viewBox: 0 0 18 18
     * Simple X shape for close/cancel actions
     */
    static juce::Path createCloseIcon() {
        juce::Path path;
        // Top-left to bottom-right diagonal
        path.startNewSubPath (2.0f, 2.0f);
        path.lineTo (16.0f, 16.0f);
        // Top-right to bottom-left diagonal
        path.startNewSubPath (16.0f, 2.0f);
        path.lineTo (2.0f, 16.0f);
        return path;
    }

    /**
     * Creates a placeholder icon 1 for view toggle (temporary - using keyboard icon)
     * TODO: Replace with proper piano roll icon
     */
    static juce::Path createViewToggleIcon1() {
        return createKeyboardIcon();
    }

    /**
     * Creates icon 2 for view toggle (bar chart for MIDI data view)
     */
    static juce::Path createViewToggleIcon2() {
        return createBarChartIcon();
    }

    /**
     * Creates a chevron down icon for dropdowns
     * Original SVG viewBox: 0 0 12 8
     * Simple downward pointing chevron/arrow
     */
    static juce::Path createChevronDownIcon() {
        juce::Path path;
        path.startNewSubPath (1.0f, 1.0f);
        path.lineTo (6.0f, 6.0f);
        path.lineTo (11.0f, 1.0f);
        return path;
    }

    /**
     * Creates a checkmark icon for selection indicators
     * Original SVG viewBox: 0 0 12 10
     * Simple checkmark shape
     */
    static juce::Path createCheckmarkIcon() {
        juce::Path path;
        path.startNewSubPath (1.0f, 5.0f);
        path.lineTo (4.5f, 8.5f);
        path.lineTo (11.0f, 1.5f);
        return path;
    }

    /**
     * Creates the duplicate icon (two overlapping rectangles)
     * Original SVG viewBox: 0 0 21 21
     * Represents duplication/copy action
     */
    static juce::Path createDupIcon() {
        juce::Path path;
        path.startNewSubPath (20.1f, 0.0f);
        path.lineTo (6.1f, 0.0f);
        path.cubicTo (5.9f, 0.0f, 5.7f, 0.1f, 5.5f, 0.3f);
        path.cubicTo (5.3f, 0.4f, 5.2f, 0.6f, 5.2f, 0.9f);
        path.lineTo (5.2f, 5.2f);
        path.lineTo (0.9f, 5.2f);
        path.cubicTo (0.6f, 5.2f, 0.4f, 5.3f, 0.3f, 5.5f);
        path.cubicTo (0.1f, 5.7f, 0.0f, 5.9f, 0.0f, 6.1f);
        path.lineTo (0.0f, 20.1f);
        path.cubicTo (0.0f, 20.4f, 0.1f, 20.6f, 0.3f, 20.7f);
        path.cubicTo (0.4f, 20.9f, 0.6f, 21.0f, 0.9f, 21.0f);
        path.lineTo (14.9f, 21.0f);
        path.cubicTo (15.1f, 21.0f, 15.3f, 20.9f, 15.5f, 20.7f);
        path.cubicTo (15.7f, 20.6f, 15.8f, 20.4f, 15.8f, 20.1f);
        path.lineTo (15.8f, 15.8f);
        path.lineTo (20.1f, 15.8f);
        path.cubicTo (20.4f, 15.8f, 20.6f, 15.7f, 20.7f, 15.5f);
        path.cubicTo (20.9f, 15.3f, 21.0f, 15.1f, 21.0f, 14.9f);
        path.lineTo (21.0f, 0.9f);
        path.cubicTo (21.0f, 0.6f, 20.9f, 0.4f, 20.7f, 0.3f);
        path.cubicTo (20.6f, 0.1f, 20.4f, 0.0f, 20.1f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath (14.0f, 19.2f);
        path.lineTo (1.8f, 19.2f);
        path.lineTo (1.8f, 7.0f);
        path.lineTo (14.0f, 7.0f);
        path.lineTo (14.0f, 19.2f);
        path.closeSubPath();
        path.startNewSubPath (19.2f, 14.0f);
        path.lineTo (15.8f, 14.0f);
        path.lineTo (15.8f, 6.1f);
        path.cubicTo (15.8f, 5.9f, 15.7f, 5.7f, 15.5f, 5.5f);
        path.cubicTo (15.3f, 5.3f, 15.1f, 5.2f, 14.9f, 5.2f);
        path.lineTo (7.0f, 5.2f);
        path.lineTo (7.0f, 1.8f);
        path.lineTo (19.2f, 1.8f);
        path.lineTo (19.2f, 14.0f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the trash/delete icon (trash can with lid)
     * Original SVG viewBox: 0 0 21 22.8
     * Represents delete/clear action
     */
    static juce::Path createTrashIcon() {
        juce::Path path;
        path.startNewSubPath (20.1f, 3.5f);
        path.lineTo (15.8f, 3.5f);
        path.lineTo (15.8f, 2.6f);
        path.cubicTo (15.8f, 1.9f, 15.5f, 1.3f, 15.0f, 0.8f);
        path.cubicTo (14.5f, 0.3f, 13.8f, 0.0f, 13.1f, 0.0f);
        path.lineTo (7.9f, 0.0f);
        path.cubicTo (7.2f, 0.0f, 6.5f, 0.3f, 6.0f, 0.8f);
        path.cubicTo (5.5f, 1.3f, 5.2f, 1.9f, 5.2f, 2.6f);
        path.lineTo (5.2f, 3.5f);
        path.lineTo (0.9f, 3.5f);
        path.cubicTo (0.6f, 3.5f, 0.4f, 3.6f, 0.3f, 3.8f);
        path.cubicTo (0.1f, 3.9f, 0.0f, 4.1f, 0.0f, 4.4f);
        path.cubicTo (0.0f, 4.6f, 0.1f, 4.8f, 0.3f, 5.0f);
        path.cubicTo (0.4f, 5.2f, 0.6f, 5.2f, 0.9f, 5.2f);
        path.lineTo (1.8f, 5.2f);
        path.lineTo (1.8f, 21.0f);
        path.cubicTo (1.8f, 21.5f, 1.9f, 21.9f, 2.3f, 22.2f);
        path.cubicTo (2.6f, 22.6f, 3.0f, 22.8f, 3.5f, 22.8f);
        path.lineTo (17.5f, 22.8f);
        path.cubicTo (18.0f, 22.8f, 18.4f, 22.6f, 18.7f, 22.2f);
        path.cubicTo (19.1f, 21.9f, 19.2f, 21.5f, 19.2f, 21.0f);
        path.lineTo (19.2f, 5.2f);
        path.lineTo (20.1f, 5.2f);
        path.cubicTo (20.4f, 5.2f, 20.6f, 5.2f, 20.7f, 5.0f);
        path.cubicTo (20.9f, 4.8f, 21.0f, 4.6f, 21.0f, 4.4f);
        path.cubicTo (21.0f, 4.1f, 20.9f, 3.9f, 20.7f, 3.8f);
        path.cubicTo (20.6f, 3.6f, 20.4f, 3.5f, 20.1f, 3.5f);
        path.closeSubPath();
        path.startNewSubPath (7.0f, 2.6f);
        path.cubicTo (7.0f, 2.4f, 7.1f, 2.2f, 7.3f, 2.0f);
        path.cubicTo (7.4f, 1.8f, 7.6f, 1.8f, 7.9f, 1.8f);
        path.lineTo (13.1f, 1.8f);
        path.cubicTo (13.4f, 1.8f, 13.6f, 1.8f, 13.7f, 2.0f);
        path.cubicTo (13.9f, 2.2f, 14.0f, 2.4f, 14.0f, 2.6f);
        path.lineTo (14.0f, 3.5f);
        path.lineTo (7.0f, 3.5f);
        path.lineTo (7.0f, 2.6f);
        path.closeSubPath();
        path.startNewSubPath (17.5f, 21.0f);
        path.lineTo (3.5f, 21.0f);
        path.lineTo (3.5f, 5.2f);
        path.lineTo (17.5f, 5.2f);
        path.lineTo (17.5f, 21.0f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 9.6f);
        path.lineTo (8.8f, 16.6f);
        path.cubicTo (8.8f, 16.9f, 8.7f, 17.1f, 8.5f, 17.2f);
        path.cubicTo (8.3f, 17.4f, 8.1f, 17.5f, 7.9f, 17.5f);
        path.cubicTo (7.6f, 17.5f, 7.4f, 17.4f, 7.3f, 17.2f);
        path.cubicTo (7.1f, 17.1f, 7.0f, 16.9f, 7.0f, 16.6f);
        path.lineTo (7.0f, 9.6f);
        path.cubicTo (7.0f, 9.4f, 7.1f, 9.2f, 7.3f, 9.0f);
        path.cubicTo (7.4f, 8.8f, 7.6f, 8.8f, 7.9f, 8.8f);
        path.cubicTo (8.1f, 8.8f, 8.3f, 8.8f, 8.5f, 9.0f);
        path.cubicTo (8.7f, 9.2f, 8.8f, 9.4f, 8.8f, 9.6f);
        path.closeSubPath();
        path.startNewSubPath (14.0f, 9.6f);
        path.lineTo (14.0f, 16.6f);
        path.cubicTo (14.0f, 16.9f, 13.9f, 17.1f, 13.7f, 17.2f);
        path.cubicTo (13.6f, 17.4f, 13.4f, 17.5f, 13.1f, 17.5f);
        path.cubicTo (12.9f, 17.5f, 12.7f, 17.4f, 12.5f, 17.2f);
        path.cubicTo (12.3f, 17.1f, 12.2f, 16.9f, 12.2f, 16.6f);
        path.lineTo (12.2f, 9.6f);
        path.cubicTo (12.2f, 9.4f, 12.3f, 9.2f, 12.5f, 9.0f);
        path.cubicTo (12.7f, 8.8f, 12.9f, 8.8f, 13.1f, 8.8f);
        path.cubicTo (13.4f, 8.8f, 13.6f, 8.8f, 13.7f, 9.0f);
        path.cubicTo (13.9f, 9.2f, 14.0f, 9.4f, 14.0f, 9.6f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the eraser icon (tilted eraser shape)
     * Original SVG viewBox: 0 0 22.8 21
     * Represents erase/clear single item action
     */
    static juce::Path createEraserIcon() {
        juce::Path path;
        path.startNewSubPath (22.0f, 5.3f);
        path.lineTo (17.5f, 0.8f);
        path.cubicTo (17.0f, 0.3f, 16.3f, 0.0f, 15.6f, 0.0f);
        path.cubicTo (14.9f, 0.0f, 14.2f, 0.3f, 13.7f, 0.8f);
        path.lineTo (0.8f, 13.7f);
        path.cubicTo (0.3f, 14.2f, 0.0f, 14.9f, 0.0f, 15.6f);
        path.cubicTo (0.0f, 16.3f, 0.3f, 17.0f, 0.8f, 17.5f);
        path.lineTo (4.1f, 20.7f);
        path.cubicTo (4.1f, 20.8f, 4.2f, 20.9f, 4.3f, 20.9f);
        path.cubicTo (4.4f, 21.0f, 4.6f, 21.0f, 4.7f, 21.0f);
        path.lineTo (21.0f, 21.0f);
        path.cubicTo (21.2f, 21.0f, 21.5f, 20.9f, 21.6f, 20.7f);
        path.cubicTo (21.8f, 20.6f, 21.9f, 20.4f, 21.9f, 20.1f);
        path.cubicTo (21.9f, 19.9f, 21.8f, 19.7f, 21.6f, 19.5f);
        path.cubicTo (21.5f, 19.3f, 21.2f, 19.3f, 21.0f, 19.3f);
        path.lineTo (11.7f, 19.3f);
        path.lineTo (22.0f, 9.0f);
        path.cubicTo (22.5f, 8.5f, 22.8f, 7.8f, 22.8f, 7.2f);
        path.cubicTo (22.8f, 6.5f, 22.5f, 5.8f, 22.0f, 5.3f);
        path.closeSubPath();
        path.startNewSubPath (9.3f, 19.3f);
        path.lineTo (5.0f, 19.3f);
        path.lineTo (2.0f, 16.2f);
        path.cubicTo (1.8f, 16.1f, 1.8f, 15.8f, 1.8f, 15.6f);
        path.cubicTo (1.8f, 15.4f, 1.8f, 15.1f, 2.0f, 15.0f);
        path.lineTo (7.9f, 9.1f);
        path.lineTo (13.6f, 14.9f);
        path.lineTo (9.3f, 19.3f);
        path.closeSubPath();
        path.startNewSubPath (20.7f, 7.8f);
        path.lineTo (14.9f, 13.6f);
        path.lineTo (9.1f, 7.9f);
        path.lineTo (15.0f, 2.0f);
        path.cubicTo (15.1f, 1.9f, 15.2f, 1.9f, 15.3f, 1.8f);
        path.cubicTo (15.4f, 1.8f, 15.5f, 1.8f, 15.6f, 1.8f);
        path.cubicTo (15.7f, 1.8f, 15.8f, 1.8f, 15.9f, 1.8f);
        path.cubicTo (16.0f, 1.9f, 16.1f, 1.9f, 16.2f, 2.0f);
        path.lineTo (20.8f, 6.5f);
        path.cubicTo (20.9f, 6.7f, 21.0f, 6.9f, 21.0f, 7.2f);
        path.cubicTo (21.0f, 7.4f, 20.9f, 7.6f, 20.8f, 7.8f);
        path.lineTo (20.7f, 7.8f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the dice with one dot icon
     * Original SVG viewBox: 0 0 21 21
     * Represents randomize single step action
     */
    static juce::Path createDiceOneIcon() {
        juce::Path path;
        path.startNewSubPath (17.5f, 0.0f);
        path.lineTo (3.5f, 0.0f);
        path.cubicTo (2.6f, 0.0f, 1.7f, 0.4f, 1.0f, 1.0f);
        path.cubicTo (0.4f, 1.7f, 0.0f, 2.6f, 0.0f, 3.5f);
        path.lineTo (0.0f, 17.5f);
        path.cubicTo (0.0f, 18.4f, 0.4f, 19.3f, 1.0f, 20.0f);
        path.cubicTo (1.7f, 20.6f, 2.6f, 21.0f, 3.5f, 21.0f);
        path.lineTo (17.5f, 21.0f);
        path.cubicTo (18.4f, 21.0f, 19.3f, 20.6f, 20.0f, 20.0f);
        path.cubicTo (20.6f, 19.3f, 21.0f, 18.4f, 21.0f, 17.5f);
        path.lineTo (21.0f, 3.5f);
        path.cubicTo (21.0f, 2.6f, 20.6f, 1.7f, 20.0f, 1.0f);
        path.cubicTo (19.3f, 0.4f, 18.4f, 0.0f, 17.5f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath (19.2f, 17.5f);
        path.cubicTo (19.2f, 18.0f, 19.1f, 18.4f, 18.7f, 18.7f);
        path.cubicTo (18.4f, 19.1f, 18.0f, 19.2f, 17.5f, 19.2f);
        path.lineTo (3.5f, 19.2f);
        path.cubicTo (3.0f, 19.2f, 2.6f, 19.1f, 2.3f, 18.7f);
        path.cubicTo (1.9f, 18.4f, 1.8f, 18.0f, 1.8f, 17.5f);
        path.lineTo (1.8f, 3.5f);
        path.cubicTo (1.8f, 3.0f, 1.9f, 2.6f, 2.3f, 2.3f);
        path.cubicTo (2.6f, 1.9f, 3.0f, 1.8f, 3.5f, 1.8f);
        path.lineTo (17.5f, 1.8f);
        path.cubicTo (18.0f, 1.8f, 18.4f, 1.9f, 18.7f, 2.3f);
        path.cubicTo (19.1f, 2.6f, 19.2f, 3.0f, 19.2f, 3.5f);
        path.lineTo (19.2f, 17.5f);
        path.closeSubPath();
        path.startNewSubPath (11.8f, 10.5f);
        path.cubicTo (11.8f, 10.8f, 11.7f, 11.0f, 11.6f, 11.2f);
        path.cubicTo (11.4f, 11.4f, 11.2f, 11.6f, 11.0f, 11.7f);
        path.cubicTo (10.8f, 11.8f, 10.5f, 11.8f, 10.2f, 11.8f);
        path.cubicTo (10.0f, 11.7f, 9.8f, 11.6f, 9.6f, 11.4f);
        path.cubicTo (9.4f, 11.2f, 9.3f, 11.0f, 9.2f, 10.8f);
        path.cubicTo (9.2f, 10.5f, 9.2f, 10.2f, 9.3f, 10.0f);
        path.cubicTo (9.4f, 9.8f, 9.6f, 9.6f, 9.8f, 9.4f);
        path.cubicTo (10.0f, 9.3f, 10.2f, 9.2f, 10.5f, 9.2f);
        path.cubicTo (10.8f, 9.2f, 11.2f, 9.3f, 11.4f, 9.6f);
        path.cubicTo (11.7f, 9.8f, 11.8f, 10.2f, 11.8f, 10.5f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the random all icon (dice with 9 dots in 3x3 grid)
     * Original SVG viewBox: 0 0 17.5 21.9
     * Represents randomize all/everything action
     */
    static juce::Path createRandomAllIcon() {
        juce::Path path;
        path.startNewSubPath (3.5f, 1.8f);
        path.cubicTo (3.5f, 2.1f, 3.4f, 2.4f, 3.2f, 2.7f);
        path.cubicTo (3.0f, 3.0f, 2.7f, 3.2f, 2.4f, 3.4f);
        path.cubicTo (2.1f, 3.5f, 1.7f, 3.5f, 1.4f, 3.5f);
        path.cubicTo (1.1f, 3.4f, 0.8f, 3.2f, 0.5f, 3.0f);
        path.cubicTo (0.3f, 2.7f, 0.1f, 2.4f, 0.0f, 2.1f);
        path.cubicTo (-0.0f, 1.8f, 0.0f, 1.4f, 0.1f, 1.1f);
        path.cubicTo (0.3f, 0.8f, 0.5f, 0.5f, 0.8f, 0.3f);
        path.cubicTo (1.1f, 0.1f, 1.4f, 0.0f, 1.8f, 0.0f);
        path.cubicTo (2.2f, 0.0f, 2.7f, 0.2f, 3.0f, 0.5f);
        path.cubicTo (3.3f, 0.8f, 3.5f, 1.3f, 3.5f, 1.8f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 0.0f);
        path.cubicTo (8.4f, 0.0f, 8.1f, 0.1f, 7.8f, 0.3f);
        path.cubicTo (7.5f, 0.5f, 7.3f, 0.8f, 7.1f, 1.1f);
        path.cubicTo (7.0f, 1.4f, 7.0f, 1.8f, 7.0f, 2.1f);
        path.cubicTo (7.1f, 2.4f, 7.3f, 2.7f, 7.5f, 3.0f);
        path.cubicTo (7.8f, 3.2f, 8.1f, 3.4f, 8.4f, 3.5f);
        path.cubicTo (8.7f, 3.5f, 9.1f, 3.5f, 9.4f, 3.4f);
        path.cubicTo (9.7f, 3.2f, 10.0f, 3.0f, 10.2f, 2.7f);
        path.cubicTo (10.4f, 2.4f, 10.5f, 2.1f, 10.5f, 1.8f);
        path.cubicTo (10.5f, 1.3f, 10.3f, 0.8f, 10.0f, 0.5f);
        path.cubicTo (9.7f, 0.2f, 9.2f, 0.0f, 8.8f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath (15.8f, 3.5f);
        path.cubicTo (16.1f, 3.5f, 16.4f, 3.4f, 16.7f, 3.2f);
        path.cubicTo (17.0f, 3.0f, 17.2f, 2.7f, 17.4f, 2.4f);
        path.cubicTo (17.5f, 2.1f, 17.5f, 1.7f, 17.5f, 1.4f);
        path.cubicTo (17.4f, 1.1f, 17.2f, 0.8f, 17.0f, 0.5f);
        path.cubicTo (16.7f, 0.3f, 16.4f, 0.1f, 16.1f, 0.0f);
        path.cubicTo (15.8f, -0.0f, 15.4f, 0.0f, 15.1f, 0.1f);
        path.cubicTo (14.8f, 0.3f, 14.5f, 0.5f, 14.3f, 0.8f);
        path.cubicTo (14.1f, 1.1f, 14.0f, 1.4f, 14.0f, 1.8f);
        path.cubicTo (14.0f, 2.2f, 14.2f, 2.7f, 14.5f, 3.0f);
        path.cubicTo (14.8f, 3.3f, 15.3f, 3.5f, 15.8f, 3.5f);
        path.closeSubPath();
        path.startNewSubPath (1.8f, 6.1f);
        path.cubicTo (1.4f, 6.1f, 1.1f, 6.2f, 0.8f, 6.4f);
        path.cubicTo (0.5f, 6.6f, 0.3f, 6.9f, 0.1f, 7.2f);
        path.cubicTo (0.0f, 7.5f, -0.0f, 7.9f, 0.0f, 8.2f);
        path.cubicTo (0.1f, 8.6f, 0.3f, 8.9f, 0.5f, 9.1f);
        path.cubicTo (0.8f, 9.4f, 1.1f, 9.5f, 1.4f, 9.6f);
        path.cubicTo (1.7f, 9.7f, 2.1f, 9.6f, 2.4f, 9.5f);
        path.cubicTo (2.7f, 9.4f, 3.0f, 9.1f, 3.2f, 8.8f);
        path.cubicTo (3.4f, 8.6f, 3.5f, 8.2f, 3.5f, 7.9f);
        path.cubicTo (3.5f, 7.4f, 3.3f, 7.0f, 3.0f, 6.6f);
        path.cubicTo (2.7f, 6.3f, 2.2f, 6.1f, 1.8f, 6.1f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 6.1f);
        path.cubicTo (8.4f, 6.1f, 8.1f, 6.2f, 7.8f, 6.4f);
        path.cubicTo (7.5f, 6.6f, 7.3f, 6.9f, 7.1f, 7.2f);
        path.cubicTo (7.0f, 7.5f, 7.0f, 7.9f, 7.0f, 8.2f);
        path.cubicTo (7.1f, 8.6f, 7.3f, 8.9f, 7.5f, 9.1f);
        path.cubicTo (7.8f, 9.4f, 8.1f, 9.5f, 8.4f, 9.6f);
        path.cubicTo (8.7f, 9.7f, 9.1f, 9.6f, 9.4f, 9.5f);
        path.cubicTo (9.7f, 9.4f, 10.0f, 9.1f, 10.2f, 8.8f);
        path.cubicTo (10.4f, 8.6f, 10.5f, 8.2f, 10.5f, 7.9f);
        path.cubicTo (10.5f, 7.4f, 10.3f, 7.0f, 10.0f, 6.6f);
        path.cubicTo (9.7f, 6.3f, 9.2f, 6.1f, 8.8f, 6.1f);
        path.closeSubPath();
        path.startNewSubPath (15.8f, 6.1f);
        path.cubicTo (15.4f, 6.1f, 15.1f, 6.2f, 14.8f, 6.4f);
        path.cubicTo (14.5f, 6.6f, 14.3f, 6.9f, 14.1f, 7.2f);
        path.cubicTo (14.0f, 7.5f, 14.0f, 7.9f, 14.0f, 8.2f);
        path.cubicTo (14.1f, 8.6f, 14.3f, 8.9f, 14.5f, 9.1f);
        path.cubicTo (14.8f, 9.4f, 15.1f, 9.5f, 15.4f, 9.6f);
        path.cubicTo (15.7f, 9.7f, 16.1f, 9.6f, 16.4f, 9.5f);
        path.cubicTo (16.7f, 9.4f, 17.0f, 9.1f, 17.2f, 8.8f);
        path.cubicTo (17.4f, 8.6f, 17.5f, 8.2f, 17.5f, 7.9f);
        path.cubicTo (17.5f, 7.4f, 17.3f, 7.0f, 17.0f, 6.6f);
        path.cubicTo (16.7f, 6.3f, 16.2f, 6.1f, 15.8f, 6.1f);
        path.closeSubPath();
        path.startNewSubPath (1.8f, 12.2f);
        path.cubicTo (1.4f, 12.2f, 1.1f, 12.4f, 0.8f, 12.5f);
        path.cubicTo (0.5f, 12.7f, 0.3f, 13.0f, 0.1f, 13.3f);
        path.cubicTo (0.0f, 13.7f, -0.0f, 14.0f, 0.0f, 14.3f);
        path.cubicTo (0.1f, 14.7f, 0.3f, 15.0f, 0.5f, 15.2f);
        path.cubicTo (0.8f, 15.5f, 1.1f, 15.6f, 1.4f, 15.7f);
        path.cubicTo (1.7f, 15.8f, 2.1f, 15.7f, 2.4f, 15.6f);
        path.cubicTo (2.7f, 15.5f, 3.0f, 15.3f, 3.2f, 15.0f);
        path.cubicTo (3.4f, 14.7f, 3.5f, 14.3f, 3.5f, 14.0f);
        path.cubicTo (3.5f, 13.5f, 3.3f, 13.1f, 3.0f, 12.8f);
        path.cubicTo (2.7f, 12.4f, 2.2f, 12.2f, 1.8f, 12.2f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 12.2f);
        path.cubicTo (8.4f, 12.2f, 8.1f, 12.4f, 7.8f, 12.5f);
        path.cubicTo (7.5f, 12.7f, 7.3f, 13.0f, 7.1f, 13.3f);
        path.cubicTo (7.0f, 13.7f, 7.0f, 14.0f, 7.0f, 14.3f);
        path.cubicTo (7.1f, 14.7f, 7.3f, 15.0f, 7.5f, 15.2f);
        path.cubicTo (7.8f, 15.5f, 8.1f, 15.6f, 8.4f, 15.7f);
        path.cubicTo (8.7f, 15.8f, 9.1f, 15.7f, 9.4f, 15.6f);
        path.cubicTo (9.7f, 15.5f, 10.0f, 15.3f, 10.2f, 15.0f);
        path.cubicTo (10.4f, 14.7f, 10.5f, 14.3f, 10.5f, 14.0f);
        path.cubicTo (10.5f, 13.5f, 10.3f, 13.1f, 10.0f, 12.8f);
        path.cubicTo (9.7f, 12.4f, 9.2f, 12.2f, 8.8f, 12.2f);
        path.closeSubPath();
        path.startNewSubPath (8.8f, 18.4f);
        path.cubicTo (8.4f, 18.4f, 8.1f, 18.5f, 7.8f, 18.7f);
        path.cubicTo (7.5f, 18.9f, 7.3f, 19.1f, 7.1f, 19.5f);
        path.cubicTo (7.0f, 19.8f, 7.0f, 20.1f, 7.0f, 20.5f);
        path.cubicTo (7.1f, 20.8f, 7.3f, 21.1f, 7.5f, 21.4f);
        path.cubicTo (7.8f, 21.6f, 8.1f, 21.8f, 8.4f, 21.8f);
        path.cubicTo (8.7f, 21.9f, 9.1f, 21.9f, 9.4f, 21.7f);
        path.cubicTo (9.7f, 21.6f, 10.0f, 21.4f, 10.2f, 21.1f);
        path.cubicTo (10.4f, 20.8f, 10.5f, 20.5f, 10.5f, 20.1f);
        path.cubicTo (10.5f, 19.7f, 10.3f, 19.2f, 10.0f, 18.9f);
        path.cubicTo (9.7f, 18.6f, 9.2f, 18.4f, 8.8f, 18.4f);
        path.closeSubPath();
        path.startNewSubPath (15.8f, 12.2f);
        path.cubicTo (15.4f, 12.2f, 15.1f, 12.4f, 14.8f, 12.5f);
        path.cubicTo (14.5f, 12.7f, 14.3f, 13.0f, 14.1f, 13.3f);
        path.cubicTo (14.0f, 13.7f, 14.0f, 14.0f, 14.0f, 14.3f);
        path.cubicTo (14.1f, 14.7f, 14.3f, 15.0f, 14.5f, 15.2f);
        path.cubicTo (14.8f, 15.5f, 15.1f, 15.6f, 15.4f, 15.7f);
        path.cubicTo (15.7f, 15.8f, 16.1f, 15.7f, 16.4f, 15.6f);
        path.cubicTo (16.7f, 15.5f, 17.0f, 15.3f, 17.2f, 15.0f);
        path.cubicTo (17.4f, 14.7f, 17.5f, 14.3f, 17.5f, 14.0f);
        path.cubicTo (17.5f, 13.5f, 17.3f, 13.1f, 17.0f, 12.8f);
        path.cubicTo (16.7f, 12.4f, 16.2f, 12.2f, 15.8f, 12.2f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the shuffle icon (two curved arrows crossing)
     * Original SVG viewBox: 0 0 23.6 19.3
     * Represents shuffle/randomly rearrange action
     */
    static juce::Path createShuffleIcon() {
        juce::Path path;
        path.startNewSubPath (23.4f, 15.1f);
        path.cubicTo (23.5f, 15.2f, 23.5f, 15.3f, 23.6f, 15.4f);
        path.cubicTo (23.6f, 15.5f, 23.6f, 15.6f, 23.6f, 15.8f);
        path.cubicTo (23.6f, 15.9f, 23.6f, 16.0f, 23.6f, 16.1f);
        path.cubicTo (23.5f, 16.2f, 23.5f, 16.3f, 23.4f, 16.4f);
        path.lineTo (20.7f, 19.0f);
        path.cubicTo (20.6f, 19.2f, 20.4f, 19.3f, 20.1f, 19.3f);
        path.cubicTo (19.9f, 19.3f, 19.7f, 19.2f, 19.5f, 19.0f);
        path.cubicTo (19.3f, 18.8f, 19.2f, 18.6f, 19.2f, 18.4f);
        path.cubicTo (19.2f, 18.1f, 19.3f, 17.9f, 19.5f, 17.8f);
        path.lineTo (20.6f, 16.6f);
        path.lineTo (19.4f, 16.6f);
        path.cubicTo (18.1f, 16.6f, 16.9f, 16.3f, 15.8f, 15.8f);
        path.cubicTo (14.6f, 15.2f, 13.7f, 14.3f, 12.9f, 13.3f);
        path.lineTo (8.4f, 6.9f);
        path.cubicTo (7.8f, 6.1f, 7.1f, 5.5f, 6.2f, 5.1f);
        path.cubicTo (5.3f, 4.6f, 4.4f, 4.4f, 3.4f, 4.4f);
        path.lineTo (0.9f, 4.4f);
        path.cubicTo (0.6f, 4.4f, 0.4f, 4.3f, 0.3f, 4.1f);
        path.cubicTo (0.1f, 4.0f, 0.0f, 3.7f, 0.0f, 3.5f);
        path.cubicTo (0.0f, 3.3f, 0.1f, 3.0f, 0.3f, 2.9f);
        path.cubicTo (0.4f, 2.7f, 0.6f, 2.6f, 0.9f, 2.6f);
        path.lineTo (3.4f, 2.6f);
        path.cubicTo (4.7f, 2.6f, 5.9f, 2.9f, 7.0f, 3.5f);
        path.cubicTo (8.1f, 4.1f, 9.1f, 4.9f, 9.8f, 5.9f);
        path.lineTo (14.4f, 12.3f);
        path.cubicTo (14.9f, 13.1f, 15.7f, 13.7f, 16.6f, 14.2f);
        path.cubicTo (17.4f, 14.6f, 18.4f, 14.9f, 19.4f, 14.9f);
        path.lineTo (20.6f, 14.9f);
        path.lineTo (19.5f, 13.7f);
        path.cubicTo (19.3f, 13.6f, 19.2f, 13.4f, 19.2f, 13.1f);
        path.cubicTo (19.2f, 12.9f, 19.3f, 12.7f, 19.5f, 12.5f);
        path.cubicTo (19.7f, 12.3f, 19.9f, 12.2f, 20.1f, 12.2f);
        path.cubicTo (20.4f, 12.2f, 20.6f, 12.3f, 20.7f, 12.5f);
        path.lineTo (23.4f, 15.1f);
        path.closeSubPath();
        path.startNewSubPath (13.0f, 7.3f);
        path.cubicTo (13.1f, 7.4f, 13.2f, 7.4f, 13.3f, 7.5f);
        path.cubicTo (13.4f, 7.5f, 13.6f, 7.5f, 13.7f, 7.5f);
        path.cubicTo (13.8f, 7.5f, 13.9f, 7.4f, 14.0f, 7.4f);
        path.cubicTo (14.1f, 7.3f, 14.2f, 7.2f, 14.2f, 7.1f);
        path.lineTo (14.4f, 6.9f);
        path.cubicTo (14.9f, 6.1f, 15.7f, 5.5f, 16.5f, 5.1f);
        path.cubicTo (17.4f, 4.6f, 18.4f, 4.4f, 19.4f, 4.4f);
        path.lineTo (20.6f, 4.4f);
        path.lineTo (19.5f, 5.5f);
        path.cubicTo (19.3f, 5.7f, 19.2f, 5.9f, 19.2f, 6.1f);
        path.cubicTo (19.2f, 6.4f, 19.3f, 6.6f, 19.5f, 6.7f);
        path.cubicTo (19.7f, 6.9f, 19.9f, 7.0f, 20.1f, 7.0f);
        path.cubicTo (20.4f, 7.0f, 20.6f, 6.9f, 20.7f, 6.7f);
        path.lineTo (23.4f, 4.1f);
        path.cubicTo (23.5f, 4.0f, 23.5f, 3.9f, 23.6f, 3.8f);
        path.cubicTo (23.6f, 3.7f, 23.6f, 3.6f, 23.6f, 3.5f);
        path.cubicTo (23.6f, 3.4f, 23.6f, 3.3f, 23.6f, 3.2f);
        path.cubicTo (23.5f, 3.1f, 23.5f, 3.0f, 23.4f, 2.9f);
        path.lineTo (20.7f, 0.3f);
        path.cubicTo (20.6f, 0.1f, 20.4f, -0.0f, 20.1f, 0.0f);
        path.cubicTo (19.9f, 0.0f, 19.7f, 0.1f, 19.5f, 0.3f);
        path.cubicTo (19.3f, 0.4f, 19.2f, 0.6f, 19.2f, 0.9f);
        path.cubicTo (19.2f, 1.1f, 19.3f, 1.3f, 19.5f, 1.5f);
        path.lineTo (20.6f, 2.6f);
        path.lineTo (19.4f, 2.6f);
        path.cubicTo (18.1f, 2.6f, 16.9f, 2.9f, 15.8f, 3.5f);
        path.cubicTo (14.6f, 4.1f, 13.7f, 4.9f, 12.9f, 5.9f);
        path.lineTo (12.8f, 6.1f);
        path.cubicTo (12.7f, 6.2f, 12.7f, 6.3f, 12.7f, 6.4f);
        path.cubicTo (12.6f, 6.5f, 12.6f, 6.6f, 12.7f, 6.8f);
        path.cubicTo (12.7f, 6.9f, 12.7f, 7.0f, 12.8f, 7.1f);
        path.cubicTo (12.8f, 7.2f, 12.9f, 7.3f, 13.0f, 7.3f);
        path.closeSubPath();
        path.startNewSubPath (9.7f, 11.9f);
        path.cubicTo (9.6f, 11.9f, 9.5f, 11.8f, 9.4f, 11.8f);
        path.cubicTo (9.3f, 11.8f, 9.2f, 11.8f, 9.1f, 11.8f);
        path.cubicTo (9.0f, 11.8f, 8.9f, 11.8f, 8.8f, 11.9f);
        path.cubicTo (8.7f, 12.0f, 8.6f, 12.0f, 8.5f, 12.1f);
        path.lineTo (8.4f, 12.3f);
        path.cubicTo (7.8f, 13.1f, 7.1f, 13.7f, 6.2f, 14.2f);
        path.cubicTo (5.3f, 14.6f, 4.4f, 14.9f, 3.4f, 14.9f);
        path.lineTo (0.9f, 14.9f);
        path.cubicTo (0.6f, 14.9f, 0.4f, 15.0f, 0.3f, 15.1f);
        path.cubicTo (0.1f, 15.3f, 0.0f, 15.5f, 0.0f, 15.8f);
        path.cubicTo (0.0f, 16.0f, 0.1f, 16.2f, 0.3f, 16.4f);
        path.cubicTo (0.4f, 16.5f, 0.6f, 16.6f, 0.9f, 16.6f);
        path.lineTo (3.4f, 16.6f);
        path.cubicTo (4.7f, 16.6f, 5.9f, 16.3f, 7.0f, 15.8f);
        path.cubicTo (8.1f, 15.2f, 9.1f, 14.3f, 9.8f, 13.3f);
        path.lineTo (9.9f, 13.1f);
        path.cubicTo (10.0f, 13.1f, 10.1f, 12.9f, 10.1f, 12.8f);
        path.cubicTo (10.1f, 12.7f, 10.1f, 12.6f, 10.1f, 12.5f);
        path.cubicTo (10.1f, 12.4f, 10.0f, 12.3f, 10.0f, 12.2f);
        path.cubicTo (9.9f, 12.1f, 9.8f, 12.0f, 9.7f, 11.9f);
        path.closeSubPath();
        return path;
    }

    /**
     * Create MIDI Learn icon
     * Design: Musical note with connection/link symbol
     * Size: ~18x18 coordinate space
     */
    static juce::Path createMidiLearnIcon() {
        juce::Path path;

        // Musical note (eighth note on left side)
        // Note head (filled circle)
        path.addEllipse (2.0f, 12.0f, 4.0f, 3.0f);

        // Note stem
        path.startNewSubPath (6.0f, 13.5f);
        path.lineTo (6.0f, 3.0f);

        // Note flag
        path.startNewSubPath (6.0f, 3.0f);
        path.cubicTo (8.0f, 3.0f, 9.0f, 4.5f, 9.0f, 6.5f);
        path.cubicTo (9.0f, 7.5f, 8.5f, 8.5f, 7.5f, 9.0f);

        // Connection symbol (lightning bolt / link on right side)
        // Top part of bolt
        path.startNewSubPath (12.0f, 2.0f);
        path.lineTo (14.0f, 8.0f);
        path.lineTo (11.5f, 8.0f);
        path.lineTo (14.0f, 16.0f);
        path.lineTo (11.0f, 10.0f);
        path.lineTo (13.0f, 10.0f);
        path.closeSubPath();

        return path;
    }

    /**
     * Play Icon (triangle pointing right)
     * Design: Classic play button triangle
     * Size: ~18x18 coordinate space
     */
    static juce::Path createPlayIcon() {
        juce::Path path;

        // Triangle pointing right
        path.startNewSubPath (5.0f, 3.0f);
        path.lineTo (15.0f, 9.0f);
        path.lineTo (5.0f, 15.0f);
        path.closeSubPath();

        return path;
    }

    /**
     * Stop Icon (square)
     * Design: Classic stop button square
     * Size: ~18x18 coordinate space
     */
    static juce::Path createStopIcon() {
        juce::Path path;

        // Square
        path.addRectangle (4.0f, 4.0f, 10.0f, 10.0f);

        return path;
    }
};
