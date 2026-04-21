#pragma once

#include "../PluginProcessor.h"
#include "ClickFlashMixin.h"
#include "../data/Params.h"
#include "../backend/MidiLearnRegistry/MidiLearnRegistry.h"
#include "../backend/MidiMapping/MidiMapping.h"
#include "StateListenerComponent.h"
#include "MidiLearnOverlayPainter.h"
#include "Icons.h"
#include "ToggleIconButton.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * DestructiveToggleButton
 *
 * A navbar toggle button backed by an APVTS bool parameter.
 * Displays one icon when off and another when on, with color feedback.
 * Used for "Bake scale to steps" and "Bake range to steps" toggles.
 */
class DestructiveToggleButton : public StateListenerComponent {
public:
    DestructiveToggleButton(PluginProcessor& processor,
                            const juce::String& paramId,
                            MidiControlId controlId,
                            std::function<juce::Path()> iconFunc)
        : StateListenerComponent(processor),
          paramId_(paramId),
          midiControlId_(controlId),
          createIcon_(std::move(iconFunc))
    {
        setSize(ToggleIconButtonConstants::totalSize, ToggleIconButtonConstants::totalSize);
        listenToParameter(paramId_);
    }

    void setRegistry(MidiLearnRegistry* reg) { registry_ = reg; }

    void paint(juce::Graphics& g) override {
        using namespace ToggleIconButtonConstants;

        auto path = createIcon_();
        bool on = isParamOn();
        auto color = on ? juce::Colour(0xFFFFD700) : juce::Colours::white;

        float boundsSize = static_cast<float>(juce::jmin(getWidth(), getHeight()));
        float paddingRatio = static_cast<float>(padding) / static_cast<float>(totalSize);
        float actualPadding = boundsSize * paddingRatio;
        float actualContent = boundsSize - actualPadding * 2.0f;
        auto iconArea = juce::Rectangle<float>(actualPadding, actualPadding, actualContent, actualContent);
        Icons::drawIcon(g, path, iconArea, color, true, 1.0f);

        clickFlash_.paint(g, getLocalBounds());

        if (registry_ && registry_->isLearning()) {
            MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, 0.0f);
        }
    }

    void mouseDown(const juce::MouseEvent&) override {
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
            registry_->setLearningControl(midiControlId_);
            repaint();
            return;
        }
        clickFlash_.trigger();
        setParamOn(!isParamOn());
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    bool getToggled() const { return isParamOn(); }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr)
            clickFlash_.attachTo(this);
    }

protected:
    void onStateChanged() override { repaint(); }

private:
    bool isParamOn() const {
        if (auto* param = getProcessor().getAPVTS().getParameter(paramId_))
            return param->getValue() > 0.5f;
        return false;
    }

    void setParamOn(bool on) {
        if (auto* param = getProcessor().getAPVTS().getParameter(paramId_))
            param->setValueNotifyingHost(on ? 1.0f : 0.0f);
    }

    juce::String paramId_;
    MidiControlId midiControlId_;
    std::function<juce::Path()> createIcon_;
    MidiLearnRegistry* registry_ = nullptr;
    ClickFlash clickFlash_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DestructiveToggleButton)
};
