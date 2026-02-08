#pragma once

#include "../PluginProcessor.h"
#include "../data/Params.h"
#include "StateListenerComponent.h"
#include "Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ScaleClearButtonConstants {
    constexpr int buttonSize = 36;  // Match ToggleIconButton size
    constexpr int contentSize = 20;
    constexpr int padding = 8;
}

/**
 * ScaleClearButton
 *
 * A button that clears the selected step's scale (sets all notes to enabled/chromatic).
 * This is a momentary button - it triggers the clear action when clicked.
 *
 * Syncs with CLEAR_SCALE APVTS parameter (trigger).
 * 
 * Visual:
 * - Eraser icon
 * - Highlights on mouse down
 */
class ScaleClearButton : public StateListenerComponent {
public:
    explicit ScaleClearButton(PluginProcessor& processor)
        : StateListenerComponent(processor),
          isPressed(false)
    {
        setSize(ScaleClearButtonConstants::buttonSize, ScaleClearButtonConstants::buttonSize);

        // Listen to the CLEAR_SCALE parameter
        listenToParameter(Params::Keys::ID::CLEAR_SCALE);
    }

    void paint(juce::Graphics& g) override {
        using namespace ScaleClearButtonConstants;

        // Get the eraser icon
        juce::Path path = Icons::createEraserIcon();
        juce::Colour color = isPressed ? juce::Colours::grey : juce::Colours::white;

        // Scale and center the icon
        auto pathBounds = path.getBounds();
        float scale = static_cast<float>(contentSize) / juce::jmax(pathBounds.getWidth(), pathBounds.getHeight());

        juce::AffineTransform transform = juce::AffineTransform::scale(scale)
            .translated(
                padding + (contentSize - pathBounds.getWidth() * scale) / 2.0f - (pathBounds.getX() * scale),
                padding + (contentSize - pathBounds.getHeight() * scale) / 2.0f - (pathBounds.getY() * scale)
            );

        // Draw the icon
        g.setColour(color);
        g.fillPath(path, transform);
    }

    void mouseDown(const juce::MouseEvent&) override {
        isPressed = true;
        repaint();
        
        // Trigger the clear action by setting parameter to true
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::CLEAR_SCALE)) {
            param->setValueNotifyingHost(1.0f);
        }
    }

    void mouseUp(const juce::MouseEvent&) override {
        isPressed = false;
        repaint();
        
        // Reset the parameter to false
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::CLEAR_SCALE)) {
            param->setValueNotifyingHost(0.0f);
        }
    }

protected:
    void onStateChanged() override {
        // Parameter changed - repaint to reflect new state
        repaint();
    }

private:
    bool isPressed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleClearButton)
};

