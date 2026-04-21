#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Icons - Centralized icon system for the application.
 *
 * Icons live in namespace Icons. Per-category path factories are split
 * across files under Icons/ (Sequencer, Transport, Navigation, Scale,
 * View, Logo) and aggregated here.
 *
 * Usage:
 *   auto path = Icons::createDiceIcon();
 *   Icons::drawIcon(g, path, bounds, juce::Colours::white);
 */
namespace Icons {

inline juce::Colour defaultColor() { return juce::Colours::white; }

inline constexpr float closeIconScale = 0.75f;

/** Create a transform that centers and scales an icon path into target bounds. */
inline juce::AffineTransform createTransform (const juce::Path& iconPath,
                                              const juce::Rectangle<float>& targetBounds,
                                              bool maintainAspectRatio = true,
                                              float scaleFactor = 1.0f) {
    auto pathBounds = iconPath.getBounds();

    const float scaleX = targetBounds.getWidth() / pathBounds.getWidth();
    const float scaleY = targetBounds.getHeight() / pathBounds.getHeight();
    float scale = maintainAspectRatio ? juce::jmin (scaleX, scaleY) : scaleX;
    scale *= scaleFactor;

    const float scaledWidth = pathBounds.getWidth() * scale;
    const float scaledHeight = pathBounds.getHeight() * scale;
    const float offsetX = targetBounds.getX() + (targetBounds.getWidth() - scaledWidth) * 0.5f;
    const float offsetY = targetBounds.getY() + (targetBounds.getHeight() - scaledHeight) * 0.5f;

    return juce::AffineTransform::scale (scale)
        .translated (offsetX - pathBounds.getX() * scale,
                     offsetY - pathBounds.getY() * scale);
}

/** Fill-draw an icon into the given bounds with a colour. */
inline void drawIcon (juce::Graphics& g,
                      const juce::Path& iconPath,
                      const juce::Rectangle<float>& targetBounds,
                      const juce::Colour& color = juce::Colours::white,
                      bool maintainAspectRatio = true,
                      float scaleFactor = 1.0f) {
    const auto transform = createTransform (iconPath, targetBounds, maintainAspectRatio, scaleFactor);
    g.setColour (color);
    g.fillPath (iconPath, transform);
}

/** Stroke-draw an icon into the given bounds. */
inline void strokeIcon (juce::Graphics& g,
                        const juce::Path& iconPath,
                        const juce::Rectangle<float>& targetBounds,
                        const juce::Colour& color = juce::Colours::white,
                        float strokeWidth = 1.5f,
                        bool maintainAspectRatio = true,
                        float scaleFactor = 0.75f) {
    const auto transform = createTransform (iconPath, targetBounds, maintainAspectRatio, scaleFactor);
    g.setColour (color);
    g.strokePath (iconPath, juce::PathStrokeType (strokeWidth), transform);
}

} // namespace Icons

#include "Icons/SequencerIcons.h"
#include "Icons/TransportIcons.h"
#include "Icons/NavigationIcons.h"
#include "Icons/ScaleIcons.h"
#include "Icons/ViewIcons.h"
#include "Icons/LogoIcons.h"
