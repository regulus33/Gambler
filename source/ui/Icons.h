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
 *   g.setColour(Icons::defaultColoedr());
 *   g.fillPath(path, transform);
 * 
 * Or use the convenience method:
 *   Icons::drawIcon(g, Icons::createDiceIcon(), targetBounds, juce::Colours::white);
 */
class Icons {
public:
    // Default icon color
    static juce::Colour defaultColor() { return juce::Colours::white; }

    // Scale factor for close icon (adjustable constant)
    static constexpr float closeIconScale = 0.75f;

    /**
     * Create a transform to fit an icon path into target bounds
     * Centers the icon and scales it to fit within the bounds
     * @param iconPath The path to transform
     * @param targetBounds The bounds to fit the icon into
     * @param maintainAspectRatio Whether to maintain aspect ratio
     * @param scaleFactor Additional scale factor (1.0 = normal, 0.75 = 75% size, etc.)
     */
    static juce::AffineTransform createTransform (const juce::Path& iconPath,
        const juce::Rectangle<float>& targetBounds,
        bool maintainAspectRatio = true,
        float scaleFactor = 1.0f) {
        auto pathBounds = iconPath.getBounds();

        float scaleX = targetBounds.getWidth() / pathBounds.getWidth();
        float scaleY = targetBounds.getHeight() / pathBounds.getHeight();
        float scale = maintainAspectRatio ? juce::jmin (scaleX, scaleY) : scaleX;

        // Apply additional scale factor
        scale *= scaleFactor;

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
        bool maintainAspectRatio = true,
        float scaleFactor = 1.0f) {
        auto transform = createTransform (iconPath, targetBounds, maintainAspectRatio, scaleFactor);
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
        bool maintainAspectRatio = true,
        float scaleFactor = 0.75f) {
        auto transform = createTransform (iconPath, targetBounds, maintainAspectRatio, scaleFactor);
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

    // /**
    //  * Creates the close (X) icon path
    //  * Original SVG viewBox: 0 0 18 18
    //  * Simple X shape for close/cancel actions
    //  */
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
     * Creates a chevron-left icon (simple < shape)
     * ViewBox: 0 0 7 12
     */
    static juce::Path createChevronLeftIcon() {
        juce::Path path;
        path.startNewSubPath (6.0f, 1.0f);
        path.lineTo (1.0f, 6.0f);
        path.lineTo (6.0f, 11.0f);
        return path;
    }

    /**
     * Creates a chevron-right icon (simple > shape)
     * ViewBox: 0 0 7 12
     */
    static juce::Path createChevronRightIcon() {
        juce::Path path;
        path.startNewSubPath (1.0f, 1.0f);
        path.lineTo (6.0f, 6.0f);
        path.lineTo (1.0f, 11.0f);
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
     * Creates a record icon (filled circle)
     * ViewBox: 0 0 20 20
     * Simple circle that can be filled or stroked
     */
    static juce::Path createRecordIcon() {
        juce::Path path;
        path.addEllipse(2.0f, 2.0f, 16.0f, 16.0f);
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
        return createCrossHairIcon();
    }

    /**
     * Play Icon (triangle pointing right)
     * Design: Simple clean play button triangle
     * Size: ~9x9 coordinate space (half size for compact UI)
     */
    static juce::Path createPlayIcon() {
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
    static juce::Path createPauseIcon() {
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
    static juce::Path createStopIcon() {
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

    static juce::Path createCrossHairIcon() {
        juce::Path path;
        path.startNewSubPath(11.4f, 0.0f);
        path.cubicTo(9.1f, 0.0f, 6.9f, 0.7f, 5.1f, 1.9f);
        path.cubicTo(3.2f, 3.2f, 1.7f, 4.9f, 0.9f, 7.0f);
        path.cubicTo(0.0f, 9.1f, -0.2f, 11.4f, 0.2f, 13.6f);
        path.cubicTo(0.7f, 15.8f, 1.7f, 17.8f, 3.3f, 19.4f);
        path.cubicTo(4.9f, 21.0f, 6.9f, 22.1f, 9.2f, 22.5f);
        path.cubicTo(11.4f, 23.0f, 13.6f, 22.7f, 15.7f, 21.9f);
        path.cubicTo(17.8f, 21.0f, 19.6f, 19.6f, 20.8f, 17.7f);
        path.cubicTo(22.1f, 15.8f, 22.8f, 13.6f, 22.8f, 11.4f);
        path.cubicTo(22.7f, 8.4f, 21.5f, 5.5f, 19.4f, 3.3f);
        path.cubicTo(17.3f, 1.2f, 14.4f, 0.0f, 11.4f, 0.0f);
        path.closeSubPath();
        path.startNewSubPath(12.2f, 21.0f);
        path.lineTo(12.2f, 17.5f);
        path.cubicTo(12.2f, 17.3f, 12.2f, 17.0f, 12.0f, 16.9f);
        path.cubicTo(11.8f, 16.7f, 11.6f, 16.6f, 11.4f, 16.6f);
        path.cubicTo(11.1f, 16.6f, 10.9f, 16.7f, 10.8f, 16.9f);
        path.cubicTo(10.6f, 17.0f, 10.5f, 17.3f, 10.5f, 17.5f);
        path.lineTo(10.5f, 21.0f);
        path.cubicTo(8.3f, 20.8f, 6.2f, 19.8f, 4.6f, 18.2f);
        path.cubicTo(3.0f, 16.6f, 2.0f, 14.5f, 1.8f, 12.2f);
        path.lineTo(5.3f, 12.2f);
        path.cubicTo(5.5f, 12.2f, 5.7f, 12.2f, 5.9f, 12.0f);
        path.cubicTo(6.0f, 11.8f, 6.1f, 11.6f, 6.1f, 11.4f);
        path.cubicTo(6.1f, 11.1f, 6.0f, 10.9f, 5.9f, 10.8f);
        path.cubicTo(5.7f, 10.6f, 5.5f, 10.5f, 5.3f, 10.5f);
        path.lineTo(1.8f, 10.5f);
        path.cubicTo(2.0f, 8.3f, 3.0f, 6.2f, 4.6f, 4.6f);
        path.cubicTo(6.2f, 3.0f, 8.3f, 2.0f, 10.5f, 1.8f);
        path.lineTo(10.5f, 5.2f);
        path.cubicTo(10.5f, 5.5f, 10.6f, 5.7f, 10.8f, 5.9f);
        path.cubicTo(10.9f, 6.0f, 11.1f, 6.1f, 11.4f, 6.1f);
        path.cubicTo(11.6f, 6.1f, 11.8f, 6.0f, 12.0f, 5.9f);
        path.cubicTo(12.2f, 5.7f, 12.2f, 5.5f, 12.2f, 5.2f);
        path.lineTo(12.2f, 1.8f);
        path.cubicTo(14.5f, 2.0f, 16.6f, 3.0f, 18.2f, 4.6f);
        path.cubicTo(19.8f, 6.2f, 20.8f, 8.3f, 21.0f, 10.5f);
        path.lineTo(17.5f, 10.5f);
        path.cubicTo(17.3f, 10.5f, 17.0f, 10.6f, 16.9f, 10.8f);
        path.cubicTo(16.7f, 10.9f, 16.6f, 11.1f, 16.6f, 11.4f);
        path.cubicTo(16.6f, 11.6f, 16.7f, 11.8f, 16.9f, 12.0f);
        path.cubicTo(17.0f, 12.2f, 17.3f, 12.2f, 17.5f, 12.2f);
        path.lineTo(21.0f, 12.2f);
        path.cubicTo(20.8f, 14.5f, 19.8f, 16.6f, 18.2f, 18.2f);
        path.cubicTo(16.6f, 19.8f, 14.5f, 20.8f, 12.2f, 21.0f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates lock (closed) icon for scale latch
     * Simple padlock shape - closed/locked state
     * Original viewBox: 0 0 16 20
     */
    static juce::Path createLockClosedIcon() {
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
    static juce::Path createLockOpenIcon() {
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
     * Creates an export/download icon (arrow pointing down into a tray)
     * ViewBox: 0 0 24 24
     */
    static juce::Path createExportIcon() {
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
     * Creates the "Gambler" logo text as a vector path
     * Original SVG coordinate space: approx 0 0 163 68
     * Use createTransform() or drawIcon() to scale to target bounds.
     */
    static juce::Path createLogoTextIcon() {
        juce::Path path;
        path.startNewSubPath(147.8f, 33.5f);
        path.lineTo(154.2f, 33.5f);
        path.lineTo(153.6f, 35.6f);
        path.cubicTo(154.6f, 34.9f, 155.4f, 34.4f, 156.1f, 34.1f);
        path.cubicTo(157.0f, 33.7f, 158.0f, 33.5f, 158.9f, 33.5f);
        path.cubicTo(159.9f, 33.5f, 160.6f, 33.8f, 161.2f, 34.3f);
        path.cubicTo(161.5f, 34.5f, 161.6f, 34.8f, 161.8f, 35.1f);
        path.cubicTo(161.9f, 35.4f, 162.0f, 35.7f, 162.0f, 36.1f);
        path.cubicTo(162.0f, 36.4f, 161.9f, 36.7f, 161.8f, 37.0f);
        path.cubicTo(161.6f, 37.3f, 161.5f, 37.5f, 161.2f, 37.7f);
        path.cubicTo(160.9f, 38.0f, 160.6f, 38.2f, 160.1f, 38.3f);
        path.cubicTo(159.7f, 38.5f, 159.3f, 38.5f, 158.8f, 38.5f);
        path.cubicTo(158.4f, 38.5f, 158.1f, 38.5f, 157.9f, 38.4f);
        path.cubicTo(157.6f, 38.3f, 157.5f, 38.2f, 157.3f, 38.0f);
        path.cubicTo(157.2f, 37.8f, 157.1f, 37.6f, 157.1f, 37.4f);
        path.cubicTo(157.0f, 36.9f, 156.8f, 36.6f, 156.7f, 36.5f);
        path.cubicTo(156.6f, 36.4f, 156.4f, 36.3f, 156.2f, 36.3f);
        path.cubicTo(155.6f, 36.3f, 155.1f, 36.4f, 154.7f, 36.6f);
        path.cubicTo(154.2f, 36.8f, 153.7f, 37.1f, 153.0f, 37.6f);
        path.lineTo(149.5f, 50.6f);
        path.lineTo(143.1f, 50.6f);
        path.lineTo(147.8f, 33.5f);
        path.closeSubPath();
        path.startNewSubPath(134.8f, 51.1f);
        path.cubicTo(132.6f, 51.1f, 131.0f, 50.6f, 129.8f, 49.8f);
        path.cubicTo(128.5f, 48.9f, 127.9f, 47.5f, 127.9f, 45.6f);
        path.cubicTo(127.9f, 44.9f, 128.0f, 44.2f, 128.2f, 43.3f);
        path.cubicTo(128.4f, 42.5f, 128.6f, 41.6f, 129.0f, 40.7f);
        path.cubicTo(129.4f, 39.8f, 129.9f, 38.9f, 130.5f, 38.0f);
        path.cubicTo(131.1f, 37.2f, 131.9f, 36.4f, 132.7f, 35.8f);
        path.cubicTo(134.6f, 34.3f, 137.0f, 33.5f, 139.8f, 33.5f);
        path.cubicTo(141.6f, 33.5f, 142.9f, 33.8f, 143.7f, 34.5f);
        path.cubicTo(144.4f, 35.0f, 144.7f, 35.8f, 144.7f, 36.8f);
        path.cubicTo(144.7f, 37.7f, 144.4f, 38.6f, 143.9f, 39.5f);
        path.cubicTo(143.3f, 40.3f, 142.6f, 41.1f, 141.6f, 41.7f);
        path.cubicTo(140.6f, 42.4f, 139.5f, 42.9f, 138.3f, 43.3f);
        path.cubicTo(137.1f, 43.7f, 135.8f, 43.9f, 134.5f, 44.0f);
        path.cubicTo(134.5f, 44.5f, 134.4f, 45.0f, 134.4f, 45.3f);
        path.cubicTo(134.4f, 46.4f, 134.7f, 47.2f, 135.2f, 47.6f);
        path.cubicTo(135.7f, 48.0f, 136.5f, 48.2f, 137.6f, 48.2f);
        path.cubicTo(139.2f, 48.2f, 140.7f, 47.9f, 142.0f, 47.2f);
        path.cubicTo(142.5f, 46.9f, 143.1f, 46.6f, 143.6f, 46.2f);
        path.cubicTo(144.1f, 45.8f, 144.7f, 45.3f, 145.4f, 44.7f);
        path.lineTo(146.9f, 44.7f);
        path.cubicTo(145.5f, 46.6f, 143.9f, 48.0f, 142.1f, 49.1f);
        path.cubicTo(139.9f, 50.4f, 137.5f, 51.1f, 134.8f, 51.1f);
        path.closeSubPath();
        path.startNewSubPath(134.9f, 42.3f);
        path.cubicTo(135.7f, 42.3f, 136.5f, 42.1f, 137.3f, 41.8f);
        path.cubicTo(138.0f, 41.5f, 138.7f, 41.0f, 139.3f, 40.5f);
        path.cubicTo(139.9f, 40.0f, 140.4f, 39.4f, 140.8f, 38.7f);
        path.cubicTo(141.1f, 38.1f, 141.3f, 37.4f, 141.3f, 36.7f);
        path.cubicTo(141.3f, 36.2f, 141.2f, 35.9f, 141.0f, 35.7f);
        path.cubicTo(140.9f, 35.4f, 140.6f, 35.3f, 140.2f, 35.3f);
        path.cubicTo(139.6f, 35.3f, 139.1f, 35.5f, 138.6f, 35.9f);
        path.cubicTo(138.0f, 36.3f, 137.5f, 36.8f, 137.0f, 37.5f);
        path.cubicTo(136.6f, 38.2f, 136.1f, 38.9f, 135.8f, 39.8f);
        path.cubicTo(135.4f, 40.6f, 135.1f, 41.5f, 134.9f, 42.3f);
        path.closeSubPath();
        path.startNewSubPath(120.4f, 51.5f);
        path.cubicTo(119.0f, 51.5f, 117.9f, 51.2f, 117.1f, 50.5f);
        path.cubicTo(116.2f, 49.8f, 115.7f, 48.8f, 115.7f, 47.4f);
        path.cubicTo(115.7f, 46.8f, 115.8f, 46.0f, 116.1f, 45.2f);
        path.lineTo(120.8f, 28.1f);
        path.lineTo(127.3f, 27.4f);
        path.lineTo(122.2f, 45.8f);
        path.cubicTo(122.1f, 46.1f, 122.1f, 46.5f, 122.1f, 46.9f);
        path.cubicTo(122.1f, 47.4f, 122.2f, 47.7f, 122.5f, 47.9f);
        path.cubicTo(122.8f, 48.1f, 123.2f, 48.2f, 123.9f, 48.2f);
        path.cubicTo(124.3f, 48.2f, 124.7f, 48.1f, 125.1f, 48.0f);
        path.cubicTo(125.5f, 47.8f, 125.9f, 47.6f, 126.3f, 47.3f);
        path.cubicTo(127.0f, 46.8f, 127.5f, 46.1f, 127.8f, 45.2f);
        path.lineTo(129.7f, 45.2f);
        path.cubicTo(128.9f, 46.9f, 127.9f, 48.3f, 126.7f, 49.3f);
        path.cubicTo(125.7f, 50.1f, 124.6f, 50.7f, 123.3f, 51.1f);
        path.cubicTo(122.4f, 51.4f, 121.4f, 51.5f, 120.4f, 51.5f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates the second half of the "Gambler" logo text path (continued)
     * Combined with createLogoTextIcon() for the full text.
     * This is a continuation - use createLogoTextIconFull() for the complete path.
     */
    static juce::Path createLogoTextIconFull() {
        juce::Path path = createLogoTextIcon();
        path.startNewSubPath(105.5f, 52.3f);
        path.cubicTo(103.5f, 52.3f, 101.9f, 51.9f, 100.9f, 51.2f);
        path.cubicTo(99.8f, 50.4f, 99.3f, 49.3f, 99.3f, 47.9f);
        path.cubicTo(99.3f, 47.2f, 99.4f, 46.5f, 99.6f, 46.0f);
        path.lineTo(104.3f, 28.9f);
        path.lineTo(110.9f, 28.2f);
        path.lineTo(108.7f, 35.9f);
        path.cubicTo(109.6f, 35.5f, 110.3f, 35.2f, 110.9f, 35.1f);
        path.cubicTo(111.3f, 35.0f, 111.8f, 34.9f, 112.4f, 34.9f);
        path.cubicTo(114.0f, 34.9f, 115.3f, 35.4f, 116.1f, 36.2f);
        path.cubicTo(117.1f, 37.2f, 117.6f, 38.7f, 117.6f, 40.8f);
        path.cubicTo(117.6f, 41.5f, 117.5f, 42.3f, 117.3f, 43.1f);
        path.cubicTo(117.1f, 43.9f, 116.9f, 44.7f, 116.5f, 45.5f);
        path.cubicTo(115.7f, 47.5f, 114.4f, 49.0f, 112.8f, 50.2f);
        path.cubicTo(110.8f, 51.6f, 108.4f, 52.3f, 105.5f, 52.3f);
        path.closeSubPath();
        path.startNewSubPath(107.9f, 49.0f);
        path.cubicTo(108.8f, 49.0f, 109.7f, 48.5f, 110.6f, 47.6f);
        path.cubicTo(111.3f, 46.8f, 111.9f, 45.7f, 112.4f, 44.4f);
        path.cubicTo(112.9f, 43.1f, 113.1f, 41.8f, 113.1f, 40.6f);
        path.cubicTo(113.1f, 39.7f, 112.9f, 39.0f, 112.6f, 38.4f);
        path.cubicTo(112.4f, 38.1f, 112.1f, 37.8f, 111.8f, 37.6f);
        path.cubicTo(111.5f, 37.5f, 111.2f, 37.4f, 110.8f, 37.4f);
        path.cubicTo(110.3f, 37.4f, 109.8f, 37.4f, 109.4f, 37.5f);
        path.cubicTo(108.8f, 37.6f, 108.4f, 37.8f, 108.2f, 38.0f);
        path.lineTo(105.8f, 46.6f);
        path.cubicTo(105.7f, 46.9f, 105.6f, 47.2f, 105.6f, 47.4f);
        path.cubicTo(105.6f, 48.5f, 106.4f, 49.0f, 107.9f, 49.0f);
        path.closeSubPath();
        path.startNewSubPath(93.3f, 52.3f);
        path.cubicTo(91.4f, 52.3f, 90.1f, 51.9f, 89.2f, 51.1f);
        path.cubicTo(88.5f, 50.4f, 88.1f, 49.5f, 88.1f, 48.5f);
        path.cubicTo(88.1f, 47.9f, 88.2f, 47.4f, 88.3f, 46.7f);
        path.cubicTo(88.5f, 46.0f, 88.7f, 45.1f, 89.1f, 44.1f);
        path.cubicTo(89.4f, 43.0f, 89.6f, 42.1f, 89.8f, 41.5f);
        path.cubicTo(90.0f, 40.8f, 90.1f, 40.2f, 90.1f, 39.8f);
        path.cubicTo(90.1f, 38.6f, 89.5f, 38.0f, 88.5f, 38.0f);
        path.cubicTo(87.7f, 38.0f, 87.0f, 38.3f, 86.4f, 38.9f);
        path.cubicTo(85.9f, 39.4f, 85.4f, 40.2f, 85.0f, 41.2f);
        path.lineTo(82.0f, 52.1f);
        path.lineTo(75.7f, 52.1f);
        path.lineTo(78.9f, 40.3f);
        path.cubicTo(79.0f, 40.1f, 79.0f, 39.8f, 79.0f, 39.5f);
        path.cubicTo(79.0f, 39.0f, 78.9f, 38.7f, 78.8f, 38.4f);
        path.cubicTo(78.5f, 38.1f, 78.2f, 38.0f, 77.7f, 38.0f);
        path.cubicTo(76.9f, 38.0f, 76.1f, 38.3f, 75.5f, 38.9f);
        path.cubicTo(75.0f, 39.4f, 74.5f, 40.2f, 74.1f, 41.2f);
        path.lineTo(71.1f, 52.1f);
        path.lineTo(64.8f, 52.1f);
        path.lineTo(69.5f, 35.0f);
        path.lineTo(75.8f, 35.0f);
        path.lineTo(75.3f, 36.8f);
        path.cubicTo(76.2f, 36.1f, 77.1f, 35.5f, 78.2f, 35.2f);
        path.cubicTo(79.0f, 35.0f, 79.8f, 34.9f, 80.7f, 34.9f);
        path.cubicTo(82.0f, 34.9f, 82.9f, 35.1f, 83.7f, 35.5f);
        path.cubicTo(84.5f, 36.0f, 85.0f, 36.8f, 85.2f, 37.8f);
        path.cubicTo(86.2f, 36.7f, 87.3f, 35.9f, 88.5f, 35.4f);
        path.cubicTo(89.4f, 35.1f, 90.3f, 34.9f, 91.4f, 34.9f);
        path.cubicTo(92.9f, 34.9f, 94.0f, 35.2f, 94.9f, 35.8f);
        path.cubicTo(95.8f, 36.5f, 96.3f, 37.5f, 96.3f, 38.8f);
        path.cubicTo(96.3f, 39.5f, 96.1f, 40.5f, 95.7f, 41.8f);
        path.cubicTo(95.6f, 42.4f, 95.4f, 42.9f, 95.2f, 43.5f);
        path.cubicTo(94.8f, 44.6f, 94.6f, 45.5f, 94.3f, 46.4f);
        path.cubicTo(94.2f, 46.9f, 94.1f, 47.3f, 94.1f, 47.6f);
        path.cubicTo(94.1f, 48.0f, 94.3f, 48.4f, 94.5f, 48.6f);
        path.cubicTo(94.8f, 48.9f, 95.3f, 49.0f, 95.9f, 49.0f);
        path.cubicTo(96.8f, 49.0f, 97.5f, 48.7f, 98.1f, 48.2f);
        path.cubicTo(98.6f, 47.8f, 99.1f, 47.0f, 99.6f, 46.0f);
        path.lineTo(101.5f, 46.0f);
        path.cubicTo(100.4f, 48.5f, 99.0f, 50.2f, 97.3f, 51.2f);
        path.cubicTo(96.2f, 51.9f, 94.8f, 52.3f, 93.3f, 52.3f);
        path.closeSubPath();
        path.startNewSubPath(48.6f, 52.3f);
        path.cubicTo(46.9f, 52.3f, 45.5f, 51.9f, 44.5f, 51.1f);
        path.cubicTo(43.3f, 50.1f, 42.7f, 48.7f, 42.7f, 46.8f);
        path.cubicTo(42.7f, 46.0f, 42.8f, 45.2f, 43.0f, 44.3f);
        path.cubicTo(43.2f, 43.4f, 43.5f, 42.5f, 43.9f, 41.7f);
        path.cubicTo(44.8f, 39.7f, 46.1f, 38.1f, 47.7f, 36.9f);
        path.cubicTo(49.5f, 35.6f, 51.6f, 34.9f, 54.0f, 34.9f);
        path.cubicTo(55.2f, 34.9f, 56.1f, 35.0f, 56.7f, 35.4f);
        path.cubicTo(57.3f, 35.7f, 57.6f, 36.1f, 57.6f, 36.6f);
        path.lineTo(57.6f, 36.9f);
        path.lineTo(58.1f, 35.0f);
        path.lineTo(64.4f, 35.0f);
        path.lineTo(61.3f, 46.6f);
        path.cubicTo(61.1f, 47.0f, 61.1f, 47.3f, 61.1f, 47.7f);
        path.cubicTo(61.1f, 48.7f, 61.7f, 49.2f, 62.9f, 49.2f);
        path.cubicTo(63.7f, 49.2f, 64.4f, 48.9f, 65.1f, 48.3f);
        path.cubicTo(65.6f, 47.7f, 66.1f, 47.0f, 66.4f, 46.0f);
        path.lineTo(68.3f, 46.0f);
        path.cubicTo(67.5f, 47.7f, 66.6f, 49.1f, 65.4f, 50.1f);
        path.cubicTo(64.5f, 50.9f, 63.4f, 51.5f, 62.2f, 51.9f);
        path.cubicTo(61.3f, 52.2f, 60.4f, 52.3f, 59.4f, 52.3f);
        path.cubicTo(58.2f, 52.3f, 57.2f, 52.0f, 56.5f, 51.5f);
        path.cubicTo(55.7f, 51.0f, 55.3f, 50.2f, 55.1f, 49.2f);
        path.cubicTo(54.3f, 50.1f, 53.4f, 50.8f, 52.5f, 51.3f);
        path.cubicTo(51.3f, 52.0f, 50.0f, 52.3f, 48.6f, 52.3f);
        path.closeSubPath();
        path.startNewSubPath(51.4f, 49.0f);
        path.cubicTo(51.7f, 49.0f, 52.1f, 48.9f, 52.5f, 48.8f);
        path.cubicTo(52.8f, 48.7f, 53.2f, 48.5f, 53.5f, 48.2f);
        path.cubicTo(54.3f, 47.7f, 54.8f, 46.9f, 55.1f, 46.0f);
        path.lineTo(57.2f, 38.3f);
        path.cubicTo(57.2f, 38.2f, 57.2f, 38.1f, 57.1f, 37.9f);
        path.cubicTo(57.0f, 37.8f, 56.9f, 37.6f, 56.8f, 37.5f);
        path.cubicTo(56.5f, 37.2f, 56.0f, 37.0f, 55.4f, 37.0f);
        path.cubicTo(54.8f, 37.0f, 54.2f, 37.1f, 53.7f, 37.4f);
        path.cubicTo(53.1f, 37.7f, 52.6f, 38.2f, 52.1f, 38.7f);
        path.cubicTo(51.2f, 39.7f, 50.5f, 40.9f, 50.0f, 42.5f);
        path.cubicTo(49.7f, 43.2f, 49.5f, 43.8f, 49.4f, 44.5f);
        path.cubicTo(49.3f, 45.2f, 49.2f, 45.8f, 49.2f, 46.3f);
        path.cubicTo(49.2f, 47.5f, 49.5f, 48.2f, 50.0f, 48.6f);
        path.cubicTo(50.2f, 48.8f, 50.4f, 48.9f, 50.6f, 48.9f);
        path.cubicTo(50.9f, 49.0f, 51.1f, 49.0f, 51.4f, 49.0f);
        path.closeSubPath();
        path.startNewSubPath(18.8f, 67.0f);
        path.cubicTo(16.7f, 67.0f, 14.8f, 66.8f, 13.0f, 66.3f);
        path.cubicTo(11.2f, 65.9f, 9.7f, 65.2f, 8.4f, 64.4f);
        path.cubicTo(7.2f, 63.5f, 6.2f, 62.5f, 5.5f, 61.4f);
        path.cubicTo(4.8f, 60.2f, 4.5f, 58.9f, 4.5f, 57.5f);
        path.cubicTo(4.5f, 56.2f, 4.8f, 54.9f, 5.4f, 53.8f);
        path.cubicTo(6.0f, 52.6f, 6.8f, 51.6f, 7.9f, 50.7f);
        path.cubicTo(9.0f, 49.8f, 10.3f, 49.1f, 11.7f, 48.6f);
        path.cubicTo(13.2f, 48.1f, 14.8f, 47.9f, 16.4f, 47.9f);
        path.cubicTo(18.7f, 47.9f, 20.5f, 48.3f, 21.9f, 49.2f);
        path.cubicTo(23.3f, 50.0f, 24.0f, 51.2f, 24.0f, 52.7f);
        path.cubicTo(24.0f, 53.8f, 23.6f, 54.7f, 23.0f, 55.3f);
        path.cubicTo(22.5f, 54.8f, 21.7f, 54.3f, 20.8f, 54.0f);
        path.cubicTo(19.9f, 53.7f, 18.9f, 53.5f, 17.9f, 53.5f);
        path.cubicTo(16.1f, 53.5f, 14.7f, 54.0f, 13.6f, 54.9f);
        path.cubicTo(12.5f, 55.8f, 11.9f, 57.0f, 11.9f, 58.5f);
        path.cubicTo(11.9f, 60.0f, 12.5f, 61.2f, 13.6f, 62.1f);
        path.cubicTo(14.6f, 63.1f, 16.0f, 63.5f, 17.7f, 63.5f);
        path.cubicTo(19.7f, 63.5f, 21.5f, 62.8f, 23.0f, 61.4f);
        path.cubicTo(24.4f, 59.9f, 25.6f, 57.8f, 26.4f, 55.0f);
        path.lineTo(30.8f, 39.6f);
        path.lineTo(30.4f, 39.6f);
        path.cubicTo(28.8f, 41.4f, 26.8f, 42.7f, 24.6f, 43.6f);
        path.cubicTo(22.3f, 44.6f, 19.8f, 45.0f, 17.2f, 45.0f);
        path.cubicTo(14.6f, 45.0f, 12.2f, 44.6f, 10.1f, 43.8f);
        path.cubicTo(8.0f, 43.0f, 6.2f, 41.8f, 4.7f, 40.3f);
        path.cubicTo(3.2f, 38.7f, 2.0f, 36.9f, 1.2f, 34.7f);
        path.cubicTo(0.4f, 32.5f, 0.0f, 30.1f, 0.0f, 27.5f);
        path.cubicTo(0.0f, 24.9f, 0.4f, 22.4f, 1.1f, 20.1f);
        path.cubicTo(1.8f, 17.7f, 2.9f, 15.5f, 4.3f, 13.4f);
        path.cubicTo(7.2f, 9.3f, 11.0f, 6.0f, 15.9f, 3.7f);
        path.cubicTo(20.9f, 1.2f, 26.4f, 0.0f, 32.4f, 0.0f);
        path.cubicTo(34.9f, 0.0f, 37.1f, 0.2f, 39.2f, 0.7f);
        path.cubicTo(41.2f, 1.1f, 42.9f, 1.7f, 44.4f, 2.5f);
        path.cubicTo(45.8f, 3.4f, 46.9f, 4.4f, 47.6f, 5.6f);
        path.cubicTo(48.4f, 6.8f, 48.7f, 8.1f, 48.7f, 9.6f);
        path.cubicTo(48.7f, 11.5f, 48.1f, 13.1f, 46.9f, 14.4f);
        path.cubicTo(46.3f, 14.9f, 45.5f, 15.4f, 44.7f, 15.7f);
        path.cubicTo(43.8f, 16.0f, 42.9f, 16.2f, 42.0f, 16.2f);
        path.cubicTo(40.9f, 16.2f, 39.8f, 16.0f, 38.9f, 15.6f);
        path.cubicTo(37.9f, 15.3f, 37.0f, 14.7f, 36.4f, 14.0f);
        path.cubicTo(37.4f, 13.5f, 38.3f, 12.6f, 39.1f, 11.3f);
        path.cubicTo(39.8f, 10.1f, 40.2f, 8.9f, 40.2f, 7.6f);
        path.cubicTo(40.2f, 6.2f, 39.7f, 5.2f, 38.6f, 4.4f);
        path.cubicTo(37.6f, 3.6f, 36.2f, 3.3f, 34.3f, 3.3f);
        path.cubicTo(32.5f, 3.3f, 30.8f, 3.6f, 29.1f, 4.2f);
        path.cubicTo(27.4f, 4.8f, 25.8f, 5.7f, 24.3f, 6.9f);
        path.cubicTo(22.7f, 8.0f, 21.3f, 9.4f, 20.0f, 11.0f);
        path.cubicTo(18.7f, 12.5f, 17.6f, 14.2f, 16.7f, 16.1f);
        path.cubicTo(14.7f, 19.9f, 13.7f, 24.0f, 13.7f, 28.3f);
        path.cubicTo(13.7f, 31.8f, 14.4f, 34.5f, 15.8f, 36.3f);
        path.cubicTo(17.2f, 38.1f, 19.2f, 39.0f, 21.9f, 39.0f);
        path.cubicTo(25.2f, 39.0f, 28.0f, 37.7f, 30.4f, 35.1f);
        path.cubicTo(32.8f, 32.5f, 34.8f, 28.6f, 36.3f, 23.5f);
        path.lineTo(46.8f, 23.6f);
        path.lineTo(40.3f, 46.9f);
        path.cubicTo(38.4f, 53.8f, 35.7f, 58.9f, 32.3f, 62.1f);
        path.cubicTo(28.8f, 65.4f, 24.4f, 67.0f, 18.8f, 67.0f);
        path.closeSubPath();
        return path;
    }

    /**
     * Creates a "compact view" icon — two horizontal bars suggesting collapsed rows
     * Used to toggle between full view and compact (single-row) view
     */
    static juce::Path createCompactViewIcon() {
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
    static juce::Path createCompactTopIcon() {
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
    static juce::Path createSwapRowsIcon() {
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

};
