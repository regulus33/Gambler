#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>

/**
 * MidiDataPerStepSelectionComponent
 *
 * Displays MIDI CC parameter selection for each visible step.
 * Shows 7 vertical bar sliders (Reaktor-style) with CC parameter dropdowns above.
 * Size: 408 x 199 pixels (matches PianoRollComponent)
 *
 * Design reference: 398x198 box with 7 sliders (45x130px each)
 * Aspect ratio maintained for 408x199 size
 */
class MidiDataPerStepSelectionComponent : public juce::Component {
public:
    MidiDataPerStepSelectionComponent() {
        // Load Inter font
        interTypeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Create 7 CC sliders (one per visible step)
        for (int i = 0; i < 7; ++i) {
            auto* slider = new CCSlider(interTypeface);
            sliders.add(slider);
            addAndMakeVisible(slider);
        }

        // Create Select button
        addAndMakeVisible(selectButton);
    }

    void paint (juce::Graphics& g) override {
        // Background color (same as plugin background)
        g.fillAll (juce::Colour (0xff1a1a1a));
    }

    void resized() override {
        auto bounds = getLocalBounds();

        // Calculate slider dimensions based on container size
        // Design: 7 sliders evenly spaced across width
        // Total width = 408px, divide by 7 = ~58px per slider (with padding)
        int numSliders = 7;
        float totalWidth = bounds.getWidth();
        float sliderWidth = totalWidth / numSliders;

        // Position "Select" button in top-right corner
        int buttonWidth = 100;
        int buttonHeight = 40;
        int buttonPadding = 10;
        selectButton.setBounds(bounds.getWidth() - buttonWidth - buttonPadding,
                              buttonPadding,
                              buttonWidth,
                              buttonHeight);

        // Position sliders evenly across the width
        for (int i = 0; i < sliders.size(); ++i) {
            int x = i * sliderWidth;
            sliders[i]->setBounds(x, 0, sliderWidth, bounds.getHeight());
        }
    }

private:
    /**
     * CCSlider
     * Vertical bar slider (Reaktor-style) with CC parameter dropdown above
     * Shows CC parameter label at top and draggable vertical bar below
     */
    class CCSlider : public juce::Component {
    public:
        CCSlider(juce::Typeface::Ptr typeface) : interTypeface(typeface) {}

        void paint (juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat();

            // Colors
            juce::Colour dropdownBgColor = juce::Colour(0xff2a2a2a);
            juce::Colour borderColor = juce::Colours::white;
            juce::Colour textColor = juce::Colours::white;
            juce::Colour sliderBarColor = juce::Colour(0xffff6b6b);  // Red color for slider bar

            const float cornerRadius = 3.0f;
            const float borderWidth = 2.0f;

            // Calculate layout areas
            // Dropdown at top: ~30px height
            // Slider bar below: 75% of remaining height
            float dropdownHeight = 30.0f;
            float padding = 4.0f;

            auto dropdownBounds = bounds.removeFromTop(dropdownHeight).reduced(padding, padding);
            auto sliderAreaBounds = bounds.reduced(padding, padding);

            // Calculate slider bar dimensions (75% of available height)
            float sliderBarMaxHeight = sliderAreaBounds.getHeight() * 0.75f;
            float sliderBarWidth = sliderAreaBounds.getWidth() - (padding * 2);

            // Position slider bar at bottom of slider area
            float sliderBarHeight = sliderBarMaxHeight * sliderValue;  // sliderValue is 0.0 to 1.0
            auto sliderBarBounds = sliderAreaBounds.removeFromBottom(sliderBarHeight)
                                                   .withSizeKeepingCentre(sliderBarWidth, sliderBarHeight);

            // Draw dropdown box
            g.setColour(dropdownBgColor);
            g.fillRoundedRectangle(dropdownBounds, cornerRadius);

            // Draw white border on dropdown
            g.setColour(borderColor);
            float inset = borderWidth * 0.5f;
            juce::Rectangle<float> borderBounds = dropdownBounds.reduced(inset);
            float borderRadius = juce::jmax(0.0f, cornerRadius - inset);
            g.drawRoundedRectangle(borderBounds, borderRadius, borderWidth);

            // Draw dropdown text "CC120▼"
            g.setColour(textColor);
            juce::Font font(juce::FontOptions(interTypeface).withHeight(12.0f));
            font.setBold(true);
            g.setFont(font);
            g.drawText("CC120▼", dropdownBounds.toNearestInt(), juce::Justification::centred);

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

    private:
        void updateSliderValue(float mouseY) {
            auto bounds = getLocalBounds().toFloat();
            float dropdownHeight = 30.0f;
            float padding = 4.0f;

            auto sliderAreaBounds = bounds.removeFromTop(bounds.getHeight()).reduced(padding, padding);
            sliderAreaBounds.removeFromTop(dropdownHeight);

            // Calculate slider value based on mouse Y position (inverted: top = 1.0, bottom = 0.0)
            float relativeY = mouseY - sliderAreaBounds.getY();
            sliderValue = 1.0f - juce::jlimit(0.0f, 1.0f, relativeY / sliderAreaBounds.getHeight());

            repaint();

            DBG("Slider value: " + juce::String(sliderValue));
        }

        juce::Typeface::Ptr interTypeface;
        float sliderValue = 0.5f;  // 0.0 to 1.0
        bool isDragging = false;
    };

    /**
     * SelectButton
     * Button in top-right corner
     */
    class SelectButton : public juce::Component {
    public:
        void paint (juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat();

            // Colors
            juce::Colour bgColor = juce::Colour(0xff2a2a2a);
            juce::Colour borderColor = juce::Colours::white;
            juce::Colour textColor = juce::Colours::white;

            const float cornerRadius = 3.0f;
            const float borderWidth = 2.0f;

            // Fill background
            g.setColour(bgColor);
            g.fillRoundedRectangle(bounds, cornerRadius);

            // Draw white border
            g.setColour(borderColor);
            float inset = borderWidth * 0.5f;
            juce::Rectangle<float> borderBounds = bounds.reduced(inset);
            float borderRadius = juce::jmax(0.0f, cornerRadius - inset);
            g.drawRoundedRectangle(borderBounds, borderRadius, borderWidth);

            // Draw text "Select"
            g.setColour(textColor);
            g.drawText("Select", bounds.toNearestInt(), juce::Justification::centred);
        }

        void mouseDown (const juce::MouseEvent& /*event*/) override {
            // TODO: Handle select button click
            DBG("Select button clicked");
        }
    };

    juce::Typeface::Ptr interTypeface;
    juce::OwnedArray<CCSlider> sliders;
    SelectButton selectButton;
};

