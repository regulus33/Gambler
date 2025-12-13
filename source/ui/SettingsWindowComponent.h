#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace SettingsWindowConstants {
    constexpr int windowWidth = 400;
    constexpr int windowHeight = 300;
    constexpr int padding = 20;
    constexpr int controlHeight = 30;
    constexpr int controlSpacing = 15;
    constexpr int labelWidth = 120;
    constexpr juce::uint32 bgColourHex = 0xFF2D2D2D;
    constexpr float labelFontSize = 14.0f;
    constexpr float sliderTextBoxWidth = 60.0f;
}

/**
 * SettingsWindowComponent
 *
 * The content component for the settings popup window.
 * Contains controls that update plugin state in real-time.
 *
 * This component is displayed inside a DialogWindow which provides
 * the title bar and close button automatically.
 */
class SettingsWindowComponent : public juce::Component {
public:
    SettingsWindowComponent() {
        interTypeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Setup internal clock checkbox
        internalClockToggle.setButtonText("Internal clock");
        internalClockToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        internalClockToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        internalClockToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
        internalClockToggle.onClick = [this] {
            // Enable/disable tempo slider based on internal clock state
            tempoSlider.setEnabled(internalClockToggle.getToggleState());
            if (onInternalClockChanged)
                onInternalClockChanged(internalClockToggle.getToggleState());
        };
        addAndMakeVisible(internalClockToggle);

        // Setup tempo slider
        tempoSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        tempoSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false,
                                    static_cast<int>(SettingsWindowConstants::sliderTextBoxWidth),
                                    SettingsWindowConstants::controlHeight);
        tempoSlider.setRange(1.0, 1000.0, 1.0);
        tempoSlider.setValue(120.0);
        tempoSlider.setTextValueSuffix(" BPM");
        tempoSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        tempoSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        tempoSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        tempoSlider.setColour(juce::Slider::trackColourId, juce::Colours::grey);
        tempoSlider.setEnabled(false); // Disabled by default until internal clock is enabled
        tempoSlider.onValueChange = [this] {
            if (onTempoChanged)
                onTempoChanged(tempoSlider.getValue());
        };
        addAndMakeVisible(tempoSlider);

        // Setup tempo label
        tempoLabel.setText("Tempo", juce::dontSendNotification);
        tempoLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        tempoLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(tempoLabel);

        setSize(SettingsWindowConstants::windowWidth, SettingsWindowConstants::windowHeight);
    }

    void paint (juce::Graphics& g) override {
        using namespace SettingsWindowConstants;

        // Fill background
        g.fillAll(juce::Colour(bgColourHex));
    }

    void resized() override {
        using namespace SettingsWindowConstants;

        auto area = getLocalBounds().reduced(padding);

        // Internal clock checkbox row
        auto checkboxRow = area.removeFromTop(controlHeight);
        internalClockToggle.setBounds(checkboxRow);

        area.removeFromTop(controlSpacing);

        // Tempo slider row
        auto tempoRow = area.removeFromTop(controlHeight);
        tempoLabel.setBounds(tempoRow.removeFromLeft(labelWidth));
        tempoSlider.setBounds(tempoRow);
    }

    // Callbacks for state changes
    std::function<void(bool)> onInternalClockChanged;
    std::function<void(double)> onTempoChanged;

    // Accessors for current values
    bool isInternalClockEnabled() const { return internalClockToggle.getToggleState(); }
    double getTempo() const { return tempoSlider.getValue(); }

    // Setters for external state sync
    void setInternalClockEnabled(bool enabled) {
        internalClockToggle.setToggleState(enabled, juce::dontSendNotification);
        tempoSlider.setEnabled(enabled);
    }
    void setTempo(double bpm) { tempoSlider.setValue(bpm, juce::dontSendNotification); }

private:
    juce::Typeface::Ptr interTypeface;

    juce::ToggleButton internalClockToggle;
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
};

/**
 * SettingsWindow
 *
 * A DialogWindow wrapper that displays the SettingsWindowComponent.
 * Provides a native window with title bar and close button.
 */
class SettingsWindow : public juce::DialogWindow {
public:
    SettingsWindow()
        : DialogWindow("Settings",
                       juce::Colour(SettingsWindowConstants::bgColourHex),
                       true,  // escapeKeyTriggersCloseButton
                       true)  // addToDesktop
    {
        setContentOwned(new SettingsWindowComponent(), true);
        setResizable(false, false);
        setUsingNativeTitleBar(true);
    }

    /** Position the window to the right of the given component */
    void positionRelativeTo(juce::Component* parent) {
        if (parent == nullptr) {
            centreWithSize(getWidth(), getHeight());
            return;
        }

        // Get the parent's position on screen
        auto parentBounds = parent->getScreenBounds();

        // Position to the right of the parent with a small gap
        constexpr int gap = 10;
        int x = parentBounds.getRight() + gap;
        int y = parentBounds.getY();

        // Make sure the window fits on screen
        auto displayBounds = juce::Desktop::getInstance().getDisplays()
            .getDisplayForPoint(parentBounds.getCentre())->userArea;

        // If it would go off the right edge, position to the left of the parent instead
        if (x + getWidth() > displayBounds.getRight()) {
            x = parentBounds.getX() - getWidth() - gap;
        }

        // Clamp Y to screen bounds
        if (y + getHeight() > displayBounds.getBottom()) {
            y = displayBounds.getBottom() - getHeight();
        }
        if (y < displayBounds.getY()) {
            y = displayBounds.getY();
        }

        setTopLeftPosition(x, y);
    }

    void closeButtonPressed() override {
        // Notify that window is closing
        if (onClose) {
            onClose();
        }
        setVisible(false);
    }

    // Callback when window is closed
    std::function<void()> onClose;
};

