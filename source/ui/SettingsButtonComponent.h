#pragma once

#include "Icons.h"
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
        juce::Path path = Icons::createSettingsIcon();

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
};

