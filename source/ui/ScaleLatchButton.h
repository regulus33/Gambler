#pragma once

#include "../PluginProcessor.h"
#include "ClickFlashMixin.h"
#include "../data/Params.h"
#include "../backend/MidiLearnRegistry.h"
#include "../backend/MidiMapping.h"
#include "StateListenerComponent.h"
#include "MidiLearnOverlayPainter.h"
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
 * Supports MIDI learn overlay.
 */
class ScaleLatchButton : public StateListenerComponent {
public:
    explicit ScaleLatchButton(PluginProcessor& processor)
        : StateListenerComponent(processor),
          midiControlId_(MidiControlId::SCALE_LATCH)
    {
        setSize(ScaleLatchButtonConstants::buttonSize, ScaleLatchButtonConstants::buttonSize);

        // Listen to the SCALE_LATCH parameter
        listenToParameter(Params::Keys::ID::SCALE_LATCH);
    }

    void setRegistry(MidiLearnRegistry* reg) {
        registry_ = reg;
    }

    void paint(juce::Graphics& g) override {
        using namespace ScaleLatchButtonConstants;
        auto path = isParamOn() ? Icons::createLockClosedIcon() : Icons::createLockOpenIcon();
        // Derive padding and content from actual bounds so the icon scales with the component
        float boundsSize = static_cast<float>(juce::jmin(getWidth(), getHeight()));
        float paddingRatio = static_cast<float>(padding) / static_cast<float>(buttonSize);
        float actualPadding = boundsSize * paddingRatio;
        float actualContent = boundsSize - actualPadding * 2.0f;
        auto iconArea = juce::Rectangle<float>(actualPadding, actualPadding, actualContent, actualContent);
        Icons::drawIcon(g, path, iconArea, juce::Colours::white, true, 1.0f);

        // Draw click flash overlay
        clickFlash_.paint(g, getLocalBounds());

        // Paint MIDI learn overlay if in learn mode
        if (registry_ && registry_->isLearning()) {
            MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, 0.0f);
        }
    }

    void mouseDown(const juce::MouseEvent&) override {
        // Handle MIDI learn mode
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
            registry_->setLearningControl(midiControlId_);
            repaint();
            return;
        }

        // Click flash feedback
        clickFlash_.trigger();

        // Toggle the parameter
        setParamOn(!isParamOn());
    }

    // Get current toggle state from APVTS
    bool getToggled() const { return isParamOn(); }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr)
            clickFlash_.attachTo(this);
    }

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

    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;
    ClickFlash clickFlash_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScaleLatchButton)
};
