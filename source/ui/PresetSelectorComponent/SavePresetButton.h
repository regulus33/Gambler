#pragma once

#include "PresetSelectorConstants.h"
#include "../ClickFlashMixin.h"
#include "../Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * SavePresetButton
 *
 * A simple icon button that displays the save icon.
 */
class SavePresetButton : public juce::Component {
public:
    SavePresetButton() {
        saveIcon = Icons::createSaveIcon();
    }

    void paint(juce::Graphics& g) override {
        using namespace PresetSelectorConstants;

        auto iconBounds = getLocalBounds().toFloat().reduced(
            static_cast<float>(getWidth() - saveIconSize) / 2.0f,
            static_cast<float>(getHeight() - saveIconSize) / 2.0f);

        Icons::drawIcon(g, saveIcon, iconBounds, juce::Colours::white);
        clickFlash_.paint(g, getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent&) override {
        clickFlash_.trigger();
        if (onClick) onClick();
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr)
            clickFlash_.attachTo(this);
    }

    std::function<void()> onClick;

private:
    juce::Path saveIcon;
    ClickFlash clickFlash_;
};
