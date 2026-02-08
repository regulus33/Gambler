#pragma once

#include "../PluginProcessor.h"
#include "../data/Params.h"
#include "StateListenerComponent.h"
#include "Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ScaleLatchButtonConstants {
    constexpr int buttonSize = 36;  // Match ToggleIconButton size
    constexpr int contentSize = 20;
    constexpr int padding = 8;
}

/**
 * ScaleLatchButton
 *
 * A button that toggles between momentary (default) and latched scale editing modes.
 * - Unlocked (default): Keys are momentary - press to add, release to remove
 * - Locked: Keys are latched - click to toggle on/off
 *
 * Syncs with SCALE_LATCH APVTS parameter.
 * 
 * Visual:
 * - Unlocked icon when false (momentary mode)
 * - Locked icon when true (latched mode)
 */
class ScaleLatchButton : public StateListenerComponent {
public:
    explicit ScaleLatchButton(PluginProcessor& processor)
        : StateListenerComponent(processor)
    {
        setSize(ScaleLatchButtonConstants::buttonSize, ScaleLatchButtonConstants::buttonSize);

        // Listen to the SCALE_LATCH parameter
        listenToParameter(Params::Keys::ID::SCALE_LATCH);
    }

    void paint(juce::Graphics& g) override {
        using namespace ScaleLatchButtonConstants;

        // Get the appropriate icon based on latch state
        juce::Path path = isParamOn() ? Icons::createLockClosedIcon() : Icons::createLockOpenIcon();
        juce::Colour color = juce::Colours::white;

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
        g.strokePath(path, juce::PathStrokeType(1.5f), transform);
    }

    void mouseDown(const juce::MouseEvent&) override {
        // Toggle the parameter
        setParamOn(!isParamOn());
    }

    // Get current toggle state from APVTS
    bool getToggled() const { return isParamOn(); }

protected:
    void onStateChanged() override {
        // Parameter changed - repaint to reflect new state
        repaint();
    }

private:
    bool isParamOn() const {
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::SCALE_LATCH)) {
            return param->getValue() > 0.5f;
        }
        return false;
    }

    void setParamOn(bool on) {
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::SCALE_LATCH)) {
            param->setValueNotifyingHost(on ? 1.0f : 0.0f);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleLatchButton)
};

