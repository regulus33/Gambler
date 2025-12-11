#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace VerticalBarSliderConstants {
    constexpr float padding = 4.0f;
    constexpr juce::uint32 defaultBarColourHex = 0xffff6b6b;
}

/**
 * VerticalBarSlider
 *
 * Reaktor-style vertical bar slider.
 * Click and drag to adjust value from 0.0 to 1.0.
 * Bar grows upward from the bottom.
 *
 * Width and height can be set via constructor or setBounds().
 */
class VerticalBarSlider : public juce::Component {
public:
    /**
     * Constructor
     * @param width Initial width in pixels
     * @param height Initial height in pixels
     */
    VerticalBarSlider(int width = 50, int height = 150) {
        setSize(width, height);
    }

    void paint (juce::Graphics& g) override {
        using namespace VerticalBarSliderConstants;
        auto bounds = getLocalBounds().toFloat();

        // Fill background (if set)
        if (! backgroundColour.isTransparent())
            g.fillAll(backgroundColour);

        // Colours
        auto sliderBarColorToUse = barColour;
        if (sliderBarColorToUse.isTransparent())
            sliderBarColorToUse = juce::Colour(defaultBarColourHex);

        // Calculate slider bar dimensions
        auto sliderAreaBounds = bounds.reduced(padding, padding);

        // Calculate bar height based on slider value (0.0 to 1.0)
        float sliderBarHeight = sliderAreaBounds.getHeight() * sliderValue;
        float sliderBarWidth = sliderAreaBounds.getWidth();

        // Position slider bar at bottom, growing upward
        auto sliderBarBounds = sliderAreaBounds.removeFromBottom(sliderBarHeight)
                                               .withSizeKeepingCentre(sliderBarWidth, sliderBarHeight);

        // Draw slider bar
        g.setColour(sliderBarColorToUse);
        g.fillRect(sliderBarBounds);
    }

    void mouseDown (const juce::MouseEvent& event) override {
        if (isInToggleMode && isInToggleMode()) {
            if (onToggleLane)
                onToggleLane();
            return;
        }

        isDragging = true;
        updateSliderValue(event.position.y);
    }

    void mouseDrag (const juce::MouseEvent& event) override {
        if (isDragging) {
            updateSliderValue(event.position.y);
        }
    }

    void mouseUp (const juce::MouseEvent& /*event*/) override {
        isDragging = false;
    }

    // Get current slider value (0.0 to 1.0)
    float getValue() const {
        return sliderValue;
    }

    // Set slider value (0.0 to 1.0)
    void setValue(float newValue) {
        sliderValue = juce::jlimit(0.0f, 1.0f, newValue);
        repaint();
    }

    // Set background colour for the slider component
    void setBackgroundColour(juce::Colour newBackgroundColour) {
        backgroundColour = newBackgroundColour;
        repaint();
    }

    // Set bar colour (the filled vertical bar)
    void setBarColour(juce::Colour newBarColour) {
        barColour = newBarColour;
        repaint();
    }

    // Callback when value changes
    // Configure optional toggle-mode behaviour (used by MidiDataPerStepSelectionComponent)
    void setToggleModeHandlers (std::function<bool()> isToggleMode,
                                std::function<void()> onToggle)
    {
        isInToggleMode = std::move (isToggleMode);
        onToggleLane   = std::move (onToggle);
    }


    std::function<void(float)> onValueChanged;

private:
    void updateSliderValue(float mouseY) {
        using namespace VerticalBarSliderConstants;
        auto bounds = getLocalBounds().toFloat();
        auto sliderAreaBounds = bounds.reduced(padding, padding);

        // Calculate slider value based on mouse Y position (inverted: top = 1.0, bottom = 0.0)
        float relativeY = mouseY - sliderAreaBounds.getY();
        sliderValue = 1.0f - juce::jlimit(0.0f, 1.0f, relativeY / sliderAreaBounds.getHeight());

        repaint();

        if (onValueChanged) {
            onValueChanged(sliderValue);
        }

        DBG("Slider value: " + juce::String(sliderValue));
    }

    std::function<bool()> isInToggleMode;
    std::function<void()> onToggleLane;

    juce::Colour backgroundColour { juce::Colours::transparentBlack };
    juce::Colour barColour { juce::Colour (VerticalBarSliderConstants::defaultBarColourHex) };

    float sliderValue = 0.5f;  // 0.0 to 1.0
    bool isDragging = false;
};

