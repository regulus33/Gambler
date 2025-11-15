#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

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
        auto bounds = getLocalBounds().toFloat();
        
        // Colors
        juce::Colour sliderBarColor = juce::Colour(0xffff6b6b);  // Red color for slider bar
        
        // Calculate slider bar dimensions
        float padding = 4.0f;
        auto sliderAreaBounds = bounds.reduced(padding, padding);
        
        // Calculate bar height based on slider value (0.0 to 1.0)
        float sliderBarHeight = sliderAreaBounds.getHeight() * sliderValue;
        float sliderBarWidth = sliderAreaBounds.getWidth();
        
        // Position slider bar at bottom, growing upward
        auto sliderBarBounds = sliderAreaBounds.removeFromBottom(sliderBarHeight)
                                               .withSizeKeepingCentre(sliderBarWidth, sliderBarHeight);
        
        // Draw slider bar (red vertical bar)
        g.setColour(sliderBarColor);
        g.fillRect(sliderBarBounds);
    }
    
    void mouseDown (const juce::MouseEvent& event) override {
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
    
    // Callback when value changes
    std::function<void(float)> onValueChanged;
    
private:
    void updateSliderValue(float mouseY) {
        auto bounds = getLocalBounds().toFloat();
        float padding = 4.0f;
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
    
    float sliderValue = 0.5f;  // 0.0 to 1.0
    bool isDragging = false;
};

