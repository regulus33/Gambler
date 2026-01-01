#pragma once

#include "BinaryData.h"
#include "NavBarComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace SettingsScreenConstants {
    constexpr int padding = 40;  // Padding around settings content
    constexpr int controlHeight = 30;
    constexpr int controlSpacing = 15;
    constexpr int labelWidth = 120;
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;  // Match main app background
    constexpr float labelFontSize = 14.0f;
    constexpr float sliderTextBoxWidth = 60.0f;
}

/**
 * SettingsScreenComponent
 *
 * A full-screen settings view that displays:
 * - NavBarComponent at the top
 * - Settings controls in the center (play/stop, internal clock, tempo, note length, preview notes)
 *
 * This replaces the popup SettingsWindow with an inbuilt screen.
 */
class SettingsScreenComponent : public juce::Component {
public:
    SettingsScreenComponent() {
        interTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Add navbar
        addAndMakeVisible(navBar);

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
                                    static_cast<int>(SettingsScreenConstants::sliderTextBoxWidth),
                                    SettingsScreenConstants::controlHeight);
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

        // Setup follow playback checkbox (enabled by default)
        followPlaybackToggle.setButtonText("Follow playback");
        followPlaybackToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        followPlaybackToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        followPlaybackToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
        followPlaybackToggle.setToggleState(true, juce::dontSendNotification); // Enabled by default
        followPlaybackToggle.onClick = [this] {
            if (onFollowPlaybackChanged)
                onFollowPlaybackChanged(followPlaybackToggle.getToggleState());
        };
        addAndMakeVisible(followPlaybackToggle);

        // Setup Clear MIDI Mappings button
        clearMidiMappingsButton.setButtonText("Clear MIDI Mappings");
        clearMidiMappingsButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFFF9800)); // Orange
        clearMidiMappingsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        clearMidiMappingsButton.onClick = [this] {
            if (onClearMidiMappings)
                onClearMidiMappings();
        };
        addAndMakeVisible(clearMidiMappingsButton);

        // Setup note length slider
        noteLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        noteLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false,
                                         static_cast<int>(SettingsScreenConstants::sliderTextBoxWidth),
                                         SettingsScreenConstants::controlHeight);
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
    }

    void paint(juce::Graphics& g) override {
        using namespace SettingsScreenConstants;

        // Fill background
        g.fillAll(juce::Colour(bgColourHex));
    }

    void resized() override {
        using namespace SettingsScreenConstants;

        auto area = getLocalBounds();

        // Position navbar at the top
        navBar.setBounds(area.removeFromTop(NavBarConstants::height));

        // Add some vertical spacing after navbar
        area.removeFromTop(20);

        // Center the settings controls horizontally
        int contentWidth = 400;  // Fixed width for settings content
        auto contentArea = area.withSizeKeepingCentre(contentWidth, area.getHeight());
        contentArea = contentArea.reduced(padding, 0);

        // Play/Stop buttons row
        auto buttonRow = contentArea.removeFromTop(controlHeight);
        constexpr int buttonWidth = 80;
        constexpr int buttonGap = 10;
        playButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(buttonGap);
        stopButton.setBounds(buttonRow.removeFromLeft(buttonWidth));

        contentArea.removeFromTop(controlSpacing);

        // Internal clock checkbox row
        auto checkboxRow = contentArea.removeFromTop(controlHeight);
        internalClockToggle.setBounds(checkboxRow);

        contentArea.removeFromTop(controlSpacing);

        // Tempo slider row
        auto tempoRow = contentArea.removeFromTop(controlHeight);
        tempoLabel.setBounds(tempoRow.removeFromLeft(labelWidth));
        tempoSlider.setBounds(tempoRow);

        contentArea.removeFromTop(controlSpacing);

        // Note length slider row
        auto noteLengthRow = contentArea.removeFromTop(controlHeight);
        noteLengthLabel.setBounds(noteLengthRow.removeFromLeft(labelWidth));
        noteLengthSlider.setBounds(noteLengthRow);

        contentArea.removeFromTop(controlSpacing);

        // Preview notes on edit checkbox row
        auto previewRow = contentArea.removeFromTop(controlHeight);
        previewNotesToggle.setBounds(previewRow);

        contentArea.removeFromTop(controlSpacing);

        // Follow step checkbox row
        auto followStepRow = contentArea.removeFromTop(controlHeight);
        followStepToggle.setBounds(followStepRow);

        contentArea.removeFromTop(controlSpacing);

        // Follow playback checkbox row
        auto followPlaybackRow = contentArea.removeFromTop(controlHeight);
        followPlaybackToggle.setBounds(followPlaybackRow);

        contentArea.removeFromTop(controlSpacing * 2);  // Extra spacing before button

        // Clear MIDI Mappings button row
        auto clearMidiRow = contentArea.removeFromTop(controlHeight);
        constexpr int clearButtonWidth = 200;
        clearMidiMappingsButton.setBounds(clearMidiRow.removeFromLeft(clearButtonWidth));
    }

    // Accessors for navbar
    NavBarComponent& getNavBar() { return navBar; }

    // Callbacks for state changes
    std::function<void()> onPlayClicked;
    std::function<void()> onStopClicked;
    std::function<void(bool)> onInternalClockChanged;
    std::function<void(double)> onTempoChanged;
    std::function<void(double)> onNoteLengthChanged;
    std::function<void(bool)> onPreviewNotesChanged;
    std::function<void(bool)> onFollowStepChanged;
    std::function<void(bool)> onFollowPlaybackChanged;
    std::function<void()> onClearMidiMappings;

    // Accessors for current values
    bool isInternalClockEnabled() const { return internalClockToggle.getToggleState(); }
    double getTempo() const { return tempoSlider.getValue(); }
    double getNoteLength() const { return noteLengthSlider.getValue(); }
    bool isPreviewNotesEnabled() const { return previewNotesToggle.getToggleState(); }
    bool isFollowStepEnabled() const { return followStepToggle.getToggleState(); }
    bool isFollowPlaybackEnabled() const { return followPlaybackToggle.getToggleState(); }

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
    void setFollowPlaybackEnabled(bool enabled) {
        followPlaybackToggle.setToggleState(enabled, juce::dontSendNotification);
    }

private:
    juce::Typeface::Ptr interTypeface;

    NavBarComponent navBar;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::ToggleButton internalClockToggle;
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::Label noteLengthLabel;
    juce::Slider noteLengthSlider;
    juce::ToggleButton previewNotesToggle;
    juce::ToggleButton followStepToggle;
    juce::ToggleButton followPlaybackToggle;
    juce::TextButton clearMidiMappingsButton;
};

