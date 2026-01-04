#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>

namespace SettingsWindowConstants {
    constexpr int windowWidth = 400;
    constexpr int windowHeight = 350;  // Increased height for MIDI device selector
    constexpr int padding = 20;
    constexpr int controlHeight = 30;
    constexpr int controlSpacing = 15;
    constexpr int labelWidth = 120;
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;  // Match main app background
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

        // Setup Play button
        playButton.setButtonText("Play");
        playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4CAF50)); // Green
        playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        playButton.onClick = [this] {
            if (onPlayClicked)
                onPlayClicked();
        };
        addAndMakeVisible(playButton);

        // Setup Stop button
        stopButton.setButtonText("Stop");
        stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFF44336)); // Red
        stopButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        stopButton.onClick = [this] {
            if (onStopClicked)
                onStopClicked();
        };
        addAndMakeVisible(stopButton);

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

        // Setup preview notes on edit checkbox (enabled by default)
        previewNotesToggle.setButtonText("Preview notes on edit");
        previewNotesToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        previewNotesToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        previewNotesToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
        previewNotesToggle.setToggleState(true, juce::dontSendNotification); // Enabled by default
        previewNotesToggle.onClick = [this] {
            if (onPreviewNotesChanged)
                onPreviewNotesChanged(previewNotesToggle.getToggleState());
        };
        addAndMakeVisible(previewNotesToggle);

        // Setup follow step checkbox (enabled by default)
        followStepToggle.setButtonText("Follow step");
        followStepToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        followStepToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        followStepToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
        followStepToggle.setToggleState(true, juce::dontSendNotification); // Enabled by default
        followStepToggle.onClick = [this] {
            if (onFollowStepChanged)
                onFollowStepChanged(followStepToggle.getToggleState());
        };
        addAndMakeVisible(followStepToggle);

        // Setup note length slider
        noteLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        noteLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false,
                                         static_cast<int>(SettingsWindowConstants::sliderTextBoxWidth),
                                         SettingsWindowConstants::controlHeight);
        noteLengthSlider.setRange(0.1, 1.0, 0.01);  // 10% to 100% in 1% increments
        noteLengthSlider.setValue(0.9);  // Default 90%
        noteLengthSlider.setTextValueSuffix(" %");
        noteLengthSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        noteLengthSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        noteLengthSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        noteLengthSlider.setColour(juce::Slider::trackColourId, juce::Colours::grey);
        noteLengthSlider.onValueChange = [this] {
            if (onNoteLengthChanged)
                onNoteLengthChanged(noteLengthSlider.getValue());
        };
        // Custom text from value to show percentage
        noteLengthSlider.textFromValueFunction = [](double value) {
            return juce::String(static_cast<int>(value * 100.0));
        };
        // Custom value from text to parse percentage
        noteLengthSlider.valueFromTextFunction = [](const juce::String& text) {
            return text.getDoubleValue() / 100.0;
        };
        addAndMakeVisible(noteLengthSlider);

        // Setup note length label
        noteLengthLabel.setText("Note Length", juce::dontSendNotification);
        noteLengthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        noteLengthLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(noteLengthLabel);

        // Setup MIDI clock output device selector (standalone only)
        midiClockDeviceLabel.setText("MIDI Clock Out", juce::dontSendNotification);
        midiClockDeviceLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        midiClockDeviceLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(midiClockDeviceLabel);

        midiClockDeviceSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A2A));
        midiClockDeviceSelector.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        midiClockDeviceSelector.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
        midiClockDeviceSelector.setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
        midiClockDeviceSelector.onChange = [this] {
            if (onMidiClockDeviceChanged)
                onMidiClockDeviceChanged(midiClockDeviceSelector.getSelectedId() - 1);
        };
        addAndMakeVisible(midiClockDeviceSelector);

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

        // Play/Stop buttons row
        auto buttonRow = area.removeFromTop(controlHeight);
        constexpr int buttonWidth = 80;
        constexpr int buttonGap = 10;
        playButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(buttonGap);
        stopButton.setBounds(buttonRow.removeFromLeft(buttonWidth));

        area.removeFromTop(controlSpacing);

        // Internal clock checkbox row
        auto checkboxRow = area.removeFromTop(controlHeight);
        internalClockToggle.setBounds(checkboxRow);

        area.removeFromTop(controlSpacing);

        // Tempo slider row
        auto tempoRow = area.removeFromTop(controlHeight);
        tempoLabel.setBounds(tempoRow.removeFromLeft(labelWidth));
        tempoSlider.setBounds(tempoRow);

        area.removeFromTop(controlSpacing);

        // Note length slider row
        auto noteLengthRow = area.removeFromTop(controlHeight);
        noteLengthLabel.setBounds(noteLengthRow.removeFromLeft(labelWidth));
        noteLengthSlider.setBounds(noteLengthRow);

        area.removeFromTop(controlSpacing);

        // Preview notes on edit checkbox row
        auto previewRow = area.removeFromTop(controlHeight);
        previewNotesToggle.setBounds(previewRow);

        area.removeFromTop(controlSpacing);

        // Follow step checkbox row
        auto followStepRow = area.removeFromTop(controlHeight);
        followStepToggle.setBounds(followStepRow);

        area.removeFromTop(controlSpacing);

        // MIDI clock device selector row
        auto midiClockRow = area.removeFromTop(controlHeight);
        midiClockDeviceLabel.setBounds(midiClockRow.removeFromLeft(labelWidth));
        midiClockDeviceSelector.setBounds(midiClockRow);
    }

    // Callbacks for state changes
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void(bool)> onInternalClockChanged;
    std::function<void(double)> onTempoChanged;
    std::function<void(double)> onNoteLengthChanged;
    std::function<void(bool)> onPreviewNotesChanged;
    std::function<void(bool)> onFollowStepChanged;
    std::function<void(int)> onMidiClockDeviceChanged;

    // Accessors for current values
    bool isInternalClockEnabled() const { return internalClockToggle.getToggleState(); }
    double getTempo() const { return tempoSlider.getValue(); }
    double getNoteLength() const { return noteLengthSlider.getValue(); }
    bool isPreviewNotesEnabled() const { return previewNotesToggle.getToggleState(); }
    bool isFollowStepEnabled() const { return followStepToggle.getToggleState(); }

    // Setters for external state sync
    void setInternalClockEnabled(bool enabled) {
        internalClockToggle.setToggleState(enabled, juce::dontSendNotification);
        tempoSlider.setEnabled(enabled);
    }
    void setTempo(double bpm) { tempoSlider.setValue(bpm, juce::dontSendNotification); }
    void setNoteLength(double length) { noteLengthSlider.setValue(length, juce::dontSendNotification); }
    void setPreviewNotesEnabled(bool enabled) {
        previewNotesToggle.setToggleState(enabled, juce::dontSendNotification);
    }
    void setFollowStepEnabled(bool enabled) {
        followStepToggle.setToggleState(enabled, juce::dontSendNotification);
    }

    // Methods to populate and manage MIDI device list
    void refreshMidiDeviceList() {
        midiClockDeviceSelector.clear();
        auto devices = juce::MidiOutput::getAvailableDevices();

        if (devices.isEmpty()) {
            midiClockDeviceSelector.addItem("No MIDI devices found", 1);
            midiClockDeviceSelector.setEnabled(false);
        } else {
            for (int i = 0; i < devices.size(); ++i) {
                midiClockDeviceSelector.addItem(devices[i].name, i + 1);
            }
            midiClockDeviceSelector.setEnabled(true);
        }
    }

    void setSelectedMidiDevice(int deviceIndex) {
        if (deviceIndex >= 0) {
            midiClockDeviceSelector.setSelectedId(deviceIndex + 1, juce::dontSendNotification);
        }
    }

    int getSelectedMidiDevice() const {
        return midiClockDeviceSelector.getSelectedId() - 1;
    }

private:
    juce::Typeface::Ptr interTypeface;

    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::ToggleButton internalClockToggle;
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::Label noteLengthLabel;
    juce::Slider noteLengthSlider;
    juce::ToggleButton previewNotesToggle;
    juce::ToggleButton followStepToggle;
    juce::Label midiClockDeviceLabel;
    juce::ComboBox midiClockDeviceSelector;
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
        // Hide the window first, then notify parent to clean up
        setVisible(false);

        // Notify that window is closing (this may delete 'this')
        if (onClose) {
            onClose();
        }
        // DO NOT access 'this' after calling onClose() - the object may be deleted
    }

    // Callback when window is closed
    std::function<void()> onClose;
};

