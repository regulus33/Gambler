#pragma once

#include "Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace WriteScaleButtonConstants {
    constexpr int buttonSize = 36;  // 36x36px button
    constexpr int iconSize = 20;    // 20px icon
    constexpr int padding = 8;      // 8px padding
}

/**
 * WriteScaleButton
 *
 * A toggle button that enables "write scale" mode.
 * When toggled on (red filled circle), the current scale configuration
 * is written to the current step whenever the step changes.
 * When toggled off (white stroke circle), no writing occurs.
 *
 * Size: 36×36px (20px icon + 8px padding all around)
 * Background: Transparent
 * Icon: Circle - filled red when on, white stroke when off
 */
class WriteScaleButton : public juce::Component {
public:
    WriteScaleButton()
        : isToggled(false)
    {
        setSize(WriteScaleButtonConstants::buttonSize, WriteScaleButtonConstants::buttonSize);
    }

    void paint(juce::Graphics& g) override {
        using namespace WriteScaleButtonConstants;

        // Create the record icon (circle)
        juce::Path path = Icons::createRecordIcon();
        auto pathBounds = path.getBounds();

        // Calculate scale factor to fit path into icon area
        float scale = static_cast<float>(iconSize) / juce::jmax(pathBounds.getWidth(), pathBounds.getHeight());

        // Apply transform: scale and translate to center in button with padding
        juce::AffineTransform transform = juce::AffineTransform::scale(scale)
            .translated(
                padding + (iconSize - pathBounds.getWidth() * scale) / 2.0f - (pathBounds.getX() * scale),
                padding + (iconSize - pathBounds.getHeight() * scale) / 2.0f - (pathBounds.getY() * scale)
            );

        if (isToggled) {
            // Toggled ON: filled red circle
            g.setColour(juce::Colour(0xFFFF0000));  // Red
            g.fillPath(path, transform);
        } else {
            // Toggled OFF: white stroke circle with clear fill
            g.setColour(juce::Colours::white);
            g.strokePath(path, juce::PathStrokeType(1.5f), transform);
        }
    }

    void mouseDown(const juce::MouseEvent& /*event*/) override {
        isToggled = !isToggled;
        repaint();

        if (onClick) {
            onClick(isToggled);
        }
    }

    void mouseEnter(const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    // Set the toggle state programmatically (without triggering callback)
    void setToggled(bool toggled) {
        if (isToggled != toggled) {
            isToggled = toggled;
            repaint();
        }
    }

    // Get current toggle state
    bool getToggled() const { return isToggled; }

    // Callback when button is clicked
    // Parameter: true if toggled on, false if toggled off
    std::function<void(bool)> onClick;

private:
    bool isToggled;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WriteScaleButton)
};

