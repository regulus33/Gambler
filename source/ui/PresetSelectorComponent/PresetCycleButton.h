#pragma once

#include "PresetSelectorConstants.h"
#include "../ClickFlashMixin.h"
#include "../Icons.h"
#include "../MidiLearnOverlayPainter.h"
#include "../../backend/MidiLearnRegistry/MidiLearnRegistry.h"
#include "../../backend/MidiMapping/MidiMapping.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * PresetCycleButton
 *
 * A small icon button that draws a scaled-down left or right chevron.
 * Used to cycle through presets inside PresetSelectorComponent.
 */
class PresetCycleButton : public juce::Component {
public:
    enum class Direction { Left, Right };

    explicit PresetCycleButton(Direction dir) : direction(dir) {
        icon = (direction == Direction::Left)
            ? Icons::createChevronLeftIcon()
            : Icons::createChevronRightIcon();
    }

    void paint(juce::Graphics& g) override {
        using namespace PresetSelectorConstants;
        auto bounds = getLocalBounds().toFloat().reduced(
            static_cast<float>(getWidth()) * (1.0f - cycleIconScale) * 0.5f,
            static_cast<float>(getHeight()) * (1.0f - cycleIconScale) * 0.5f);
        auto pathBounds = icon.getBounds();
        auto transform = juce::AffineTransform::scale(
            bounds.getWidth() / pathBounds.getWidth(),
            bounds.getHeight() / pathBounds.getHeight())
            .translated(bounds.getX(), bounds.getY());
        g.setColour(juce::Colours::white);
        g.strokePath(icon, juce::PathStrokeType(chevronStrokeWidth,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded), transform);

        clickFlash_.paint(g, getLocalBounds());
        MidiLearnOverlayPainter::paint(g, getLocalBounds(), registry_, midiControlId_, 0.0f);
    }

    void mouseEnter(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    void mouseExit(const juce::MouseEvent&) override {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    void mouseUp(const juce::MouseEvent&) override {
        if (registry_ && registry_->isLearning() && midiControlId_ != MidiControlId::COUNT) {
            registry_->setLearningControl(midiControlId_);
            repaint();
            return;
        }
        clickFlash_.trigger();
        if (onClick) onClick();
    }

    void parentHierarchyChanged() override {
        if (getTopLevelComponent() != nullptr)
            clickFlash_.attachTo(this);
    }

    void setMidiControlId(MidiControlId controlId) { midiControlId_ = controlId; }
    void setRegistry(MidiLearnRegistry* reg) { registry_ = reg; }

    std::function<void()> onClick;

private:
    Direction direction;
    juce::Path icon;
    ClickFlash clickFlash_;
    MidiControlId midiControlId_ = MidiControlId::COUNT;
    MidiLearnRegistry* registry_ = nullptr;
};
