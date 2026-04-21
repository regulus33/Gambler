#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>

/** A small clickable colour swatch that opens a ColourSelector in a CallOutBox. */
class ColourSwatchComponent : public juce::Component {
public:
    ColourSwatchComponent(juce::Colour initial) : currentColour(initial) {}

    void paint(juce::Graphics& g) override {
        auto b = swatchBounds();
        g.setColour(currentColour);
        g.fillRoundedRectangle(b, 3.0f);
        g.setColour(juce::Colours::grey);
        g.drawRoundedRectangle(b.reduced(0.5f), 3.0f, 1.0f);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (!swatchBounds().contains(e.position))
            return;
        auto* selector = new juce::ColourSelector(
            juce::ColourSelector::showColourAtTop
            | juce::ColourSelector::showSliders
            | juce::ColourSelector::showColourspace);
        selector->setSize(300, 400);
        selector->setCurrentColour(currentColour);
        selector->addChangeListener(&changeForwarder);
        changeForwarder.onColourChanged = [this, selector]() {
            currentColour = selector->getCurrentColour();
            repaint();
            if (onColourChanged)
                onColourChanged(currentColour);
        };
        juce::CallOutBox::launchAsynchronously(
            std::unique_ptr<juce::Component>(selector), getScreenBounds(), getTopLevelComponent());
    }

    void setCurrentColour(juce::Colour c) { currentColour = c; repaint(); }
    juce::Colour getCurrentColour() const { return currentColour; }

    std::function<void(juce::Colour)> onColourChanged;

private:
    static constexpr float swatchSize = 15.0f;

    juce::Rectangle<float> swatchBounds() const {
        float y = (getHeight() - swatchSize) * 0.5f;
        return { 0.0f, y, swatchSize, swatchSize };
    }

    juce::Colour currentColour;

    struct ChangeForwarder : public juce::ChangeListener {
        std::function<void()> onColourChanged;
        void changeListenerCallback(juce::ChangeBroadcaster*) override {
            if (onColourChanged) onColourChanged();
        }
    };
    ChangeForwarder changeForwarder;
};
