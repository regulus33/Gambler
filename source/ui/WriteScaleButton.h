#pragma once

#include "Icons.h"
#include "StateListenerComponent.h"
#include "../data/Params.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class PluginProcessor;

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
 * Observer Pattern:
 * - Syncs with WRITE_SCALE APVTS parameter
 * - Visual state reflects parameter value
 * - PluginProcessor reads parameter in audio thread before quantization
 *
 * Size: 36×36px (20px icon + 8px padding all around)
 * Background: Transparent
 * Icon: Circle - filled red when on, white stroke when off
 */
class WriteScaleButton : public StateListenerComponent {
public:
    explicit WriteScaleButton(PluginProcessor& processor)
        : StateListenerComponent(processor)
    {
        setSize(WriteScaleButtonConstants::buttonSize, WriteScaleButtonConstants::buttonSize);

        // Listen to the WRITE_SCALE parameter
        listenToParameter(Params::Keys::ID::WRITE_SCALE);
    }

    void paint(juce::Graphics& g) override {
        using namespace WriteScaleButtonConstants;
        auto path = Icons::createRecordIcon();
        auto iconArea = juce::Rectangle<float>(padding, padding, iconSize, iconSize);

        if (isParamOn())
            Icons::drawIcon(g, path, iconArea, juce::Colour(0xFFFF0000));
        else
            Icons::strokeIcon(g, path, iconArea, juce::Colours::white, 1.5f, true, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& /*event*/) override {
        // Toggle the APVTS parameter
        bool currentState = isParamOn();
        setParamOn(!currentState);
    }

    void mouseEnter(const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent& /*event*/) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
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
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::WRITE_SCALE)) {
            return param->getValue() > 0.5f;
        }
        return false;
    }

    void setParamOn(bool on) {
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::WRITE_SCALE)) {
            param->setValueNotifyingHost(on ? 1.0f : 0.0f);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WriteScaleButton)
};

