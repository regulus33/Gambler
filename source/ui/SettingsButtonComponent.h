#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace SettingsButtonConstants {
    constexpr int contentSize = 20;      // Icon content area (20px) - 2x original
    constexpr int padding = 8;           // Padding all around (8px) - 2x original
    constexpr int totalSize = contentSize + (padding * 2);  // 36px total
}

/**
 * SettingsButtonComponent
 *
 * A settings button with a sun/gear icon.
 *
 * Size: 36×36px (20px content + 8px padding all around)
 * Background: Transparent
 * Icon: White stroked path (sun/gear icon)
 */
class SettingsButtonComponent : public juce::Component {
public:
    SettingsButtonComponent() = default;

    void paint (juce::Graphics& g) override {
        using namespace SettingsButtonConstants;

        // Create the settings icon path
        juce::Path path = createSettingsIconPath();

        // The original path is designed for a 24x24 bounding box (coords 2-22)
        // We need to scale it to fit within the contentSize (20px)
        auto pathBounds = path.getBounds();

        // Calculate scale factor to fit path into content area
        float scale = static_cast<float>(contentSize) / pathBounds.getWidth();

        // Apply transform: scale and translate to center in content area with padding
        juce::AffineTransform transform = juce::AffineTransform::scale(scale)
            .translated(
                padding - (pathBounds.getX() * scale),
                padding - (pathBounds.getY() * scale)
            );

        // Draw the icon with white stroke
        g.setColour(juce::Colours::white);
        g.strokePath(path, juce::PathStrokeType(1.5f), transform);
    }

    void mouseDown (const juce::MouseEvent& /*event*/) override {
        if (onClick) {
            onClick();
        }
    }

    void mouseEnter (const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit (const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    // Callback when button is clicked
    std::function<void()> onClick;

private:
    juce::Path createSettingsIconPath() const {
        juce::Path path;

        // Cog/gear icon - outer gear shape
        path.startNewSubPath(9.7f, 4.1f);
        path.cubicTo(9.7f, 3.6f, 10.0f, 3.0f, 10.4f, 2.6f);
        path.cubicTo(10.9f, 2.2f, 11.4f, 2.0f, 12.0f, 2.0f);
        path.cubicTo(12.6f, 2.0f, 13.1f, 2.2f, 13.6f, 2.6f);
        path.cubicTo(14.0f, 3.0f, 14.3f, 3.6f, 14.3f, 4.1f);
        path.cubicTo(14.4f, 4.5f, 14.5f, 4.9f, 14.7f, 5.2f);
        path.cubicTo(14.9f, 5.5f, 15.2f, 5.8f, 15.5f, 6.0f);
        path.cubicTo(15.8f, 6.1f, 16.2f, 6.3f, 16.6f, 6.3f);
        path.cubicTo(16.9f, 6.3f, 17.3f, 6.2f, 17.6f, 6.1f);
        path.cubicTo(18.2f, 5.8f, 18.8f, 5.8f, 19.3f, 6.0f);
        path.cubicTo(19.9f, 6.1f, 20.4f, 6.5f, 20.6f, 7.0f);
        path.cubicTo(20.9f, 7.5f, 21.0f, 8.1f, 20.9f, 8.7f);
        path.cubicTo(20.8f, 9.2f, 20.5f, 9.7f, 20.0f, 10.1f);
        path.cubicTo(19.7f, 10.3f, 19.4f, 10.6f, 19.2f, 10.9f);
        path.cubicTo(19.1f, 11.3f, 19.0f, 11.6f, 19.0f, 12.0f);
        path.cubicTo(19.0f, 12.4f, 19.1f, 12.7f, 19.2f, 13.1f);
        path.cubicTo(19.4f, 13.4f, 19.7f, 13.7f, 20.0f, 13.9f);
        path.cubicTo(20.5f, 14.3f, 20.8f, 14.8f, 20.9f, 15.3f);
        path.cubicTo(21.0f, 15.9f, 20.9f, 16.5f, 20.6f, 17.0f);
        path.cubicTo(20.4f, 17.5f, 19.9f, 17.9f, 19.3f, 18.0f);
        path.cubicTo(18.8f, 18.2f, 18.2f, 18.2f, 17.6f, 17.9f);
        path.cubicTo(17.3f, 17.8f, 16.9f, 17.7f, 16.6f, 17.7f);
        path.cubicTo(16.2f, 17.7f, 15.8f, 17.9f, 15.5f, 18.0f);
        path.cubicTo(15.2f, 18.2f, 14.9f, 18.5f, 14.7f, 18.8f);
        path.cubicTo(14.5f, 19.1f, 14.4f, 19.5f, 14.3f, 19.9f);
        path.cubicTo(14.3f, 20.4f, 14.0f, 21.0f, 13.6f, 21.4f);
        path.cubicTo(13.1f, 21.8f, 12.6f, 22.0f, 12.0f, 22.0f);
        path.cubicTo(11.4f, 22.0f, 10.9f, 21.8f, 10.4f, 21.4f);
        path.cubicTo(10.0f, 21.0f, 9.7f, 20.4f, 9.7f, 19.9f);
        path.cubicTo(9.6f, 19.5f, 9.5f, 19.1f, 9.3f, 18.8f);
        path.cubicTo(9.1f, 18.5f, 8.8f, 18.2f, 8.5f, 18.0f);
        path.cubicTo(8.2f, 17.9f, 7.8f, 17.7f, 7.4f, 17.7f);
        path.cubicTo(7.1f, 17.7f, 6.7f, 17.8f, 6.4f, 17.9f);
        path.cubicTo(5.8f, 18.2f, 5.2f, 18.2f, 4.7f, 18.0f);
        path.cubicTo(4.1f, 17.9f, 3.6f, 17.5f, 3.4f, 17.0f);
        path.cubicTo(3.1f, 16.5f, 3.0f, 15.9f, 3.1f, 15.3f);
        path.cubicTo(3.2f, 14.8f, 3.5f, 14.3f, 4.0f, 13.9f);
        path.cubicTo(4.3f, 13.7f, 4.6f, 13.4f, 4.8f, 13.1f);
        path.cubicTo(4.9f, 12.7f, 5.0f, 12.4f, 5.0f, 12.0f);
        path.cubicTo(5.0f, 11.6f, 4.9f, 11.3f, 4.8f, 10.9f);
        path.cubicTo(4.6f, 10.6f, 4.3f, 10.3f, 4.0f, 10.1f);
        path.cubicTo(3.5f, 9.7f, 3.2f, 9.2f, 3.1f, 8.7f);
        path.cubicTo(3.0f, 8.1f, 3.1f, 7.5f, 3.4f, 7.0f);
        path.cubicTo(3.6f, 6.5f, 4.1f, 6.1f, 4.7f, 6.0f);
        path.cubicTo(5.2f, 5.8f, 5.8f, 5.8f, 6.4f, 6.1f);
        path.cubicTo(6.7f, 6.2f, 7.1f, 6.3f, 7.4f, 6.3f);
        path.cubicTo(7.8f, 6.3f, 8.2f, 6.1f, 8.5f, 6.0f);
        path.cubicTo(8.8f, 5.8f, 9.1f, 5.5f, 9.3f, 5.2f);
        path.cubicTo(9.5f, 4.9f, 9.6f, 4.5f, 9.7f, 4.1f);

        // Center circle
        path.startNewSubPath(12.0f, 15.0f);
        path.cubicTo(13.7f, 15.0f, 15.0f, 13.7f, 15.0f, 12.0f);
        path.cubicTo(15.0f, 10.3f, 13.7f, 9.0f, 12.0f, 9.0f);
        path.cubicTo(10.3f, 9.0f, 9.0f, 10.3f, 9.0f, 12.0f);
        path.cubicTo(9.0f, 13.7f, 10.3f, 15.0f, 12.0f, 15.0f);
        path.closeSubPath();

        return path;
    }
};

