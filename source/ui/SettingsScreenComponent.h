#pragma once

#include "BinaryData.h"
#include "NavBarComponent.h"
#include "SettingsGroup.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>

namespace SettingsScreenConstants {
    constexpr int padding = 40;  // Padding around settings content
    constexpr int controlHeight = 30;
    constexpr int controlSpacing = 15;
    constexpr int labelWidth = 120;
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;  // Match main app background
    constexpr float labelFontSize = 14.0f;
    constexpr float sliderTextBoxWidth = 60.0f;
    constexpr int groupSpacing = 20;  // Spacing between groups
    constexpr int groupHorizontalSpacing = 20;  // Spacing between groups horizontally
    constexpr int maxGroupWidth = 500;  // Maximum width for each group
}

/**
 * SettingsScreenComponent
 *
 * A full-screen settings view that displays:
 * - NavBarComponent at the top
 * - Settings organized into logical groups with headers in a 3x2 grid:
 *   - Clock & Timing: Internal clock, tempo, MIDI clock output
 *   - Playback: Note length, preview notes, follow step
 *   - MIDI: (placeholder for future MIDI settings)
 *   - Group 4, 5, 6: (placeholders for future settings)
 *
 * Uses a 3-column grid layout that displays all groups in a 3x2 arrangement.
 * Groups are displayed in boxes with rounded corners and lighter backgrounds.
 */
class SettingsScreenComponent : public juce::Component {
public:
    SettingsScreenComponent() {
        interTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Add navbar
        addAndMakeVisible(navBar);

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

        // Setup tempo label
        tempoLabel.setText("Tempo", juce::dontSendNotification);
        tempoLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        tempoLabel.setJustificationType(juce::Justification::centredLeft);

        // Setup MIDI clock output checkbox (standalone only)
        midiClockOutputToggle.setButtonText("MIDI Clock Output");
        midiClockOutputToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        midiClockOutputToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        midiClockOutputToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
        midiClockOutputToggle.setToggleState(false, juce::dontSendNotification);
        midiClockOutputToggle.onClick = [this] {
            if (onMidiClockOutputChanged)
                onMidiClockOutputChanged(midiClockOutputToggle.getToggleState());
        };

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

        // Setup note length label
        noteLengthLabel.setText("Note Length", juce::dontSendNotification);
        noteLengthLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        noteLengthLabel.setJustificationType(juce::Justification::centredLeft);

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



        // Setup piano key momentary checkbox (disabled by default = toggle mode)
        pianoKeyMomentaryToggle.setButtonText("Piano keys momentary");
        pianoKeyMomentaryToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        pianoKeyMomentaryToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        pianoKeyMomentaryToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
        pianoKeyMomentaryToggle.setToggleState(false, juce::dontSendNotification); // Disabled by default (toggle mode)
        pianoKeyMomentaryToggle.onClick = [this] {
            if (onPianoKeyMomentaryChanged)
                onPianoKeyMomentaryChanged(pianoKeyMomentaryToggle.getToggleState());
        };

        // Create settings groups
        clockTimingGroup = std::make_unique<SettingsGroup>("Clock & Timing");
        clockTimingGroup->addControl(internalClockToggle);
        clockTimingGroup->addLabeledControl(tempoLabel, tempoSlider);
        clockTimingGroup->addControl(midiClockOutputToggle);
        addAndMakeVisible(*clockTimingGroup);

        playbackGroup = std::make_unique<SettingsGroup>("Playback");
        playbackGroup->addLabeledControl(noteLengthLabel, noteLengthSlider);
        playbackGroup->addControl(previewNotesToggle);
        playbackGroup->addControl(followStepToggle);
        addAndMakeVisible(*playbackGroup);

        // TODO: Replace placeholder text with actual MIDI settings/controls
        midiGroup = std::make_unique<SettingsGroup>("MIDI");
        midiPlaceholderLabel.setText("MIDI settings coming soon...", juce::dontSendNotification);
        midiPlaceholderLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        midiPlaceholderLabel.setJustificationType(juce::Justification::centred);
        midiGroup->addControl(midiPlaceholderLabel);
        addAndMakeVisible(*midiGroup);

        // TODO: Replace placeholder text with actual Group 4 settings/controls
        group4 = std::make_unique<SettingsGroup>("Group 4");
        group4PlaceholderLabel.setText("Group 4 settings coming soon...", juce::dontSendNotification);
        group4PlaceholderLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        group4PlaceholderLabel.setJustificationType(juce::Justification::centred);
        group4->addControl(group4PlaceholderLabel);
        addAndMakeVisible(*group4);

        // TODO: Replace placeholder text with actual Group 5 settings/controls
        group5 = std::make_unique<SettingsGroup>("Group 5");
        group5PlaceholderLabel.setText("Group 5 settings coming soon...", juce::dontSendNotification);
        group5PlaceholderLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        group5PlaceholderLabel.setJustificationType(juce::Justification::centred);
        group5->addControl(group5PlaceholderLabel);
        addAndMakeVisible(*group5);

        // Group 6: Piano Roll Settings
        group6 = std::make_unique<SettingsGroup>("Piano Roll");
        group6->addControl(pianoKeyMomentaryToggle);
        addAndMakeVisible(*group6);
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
        area.removeFromTop(30);

        // Create content area with padding
        auto contentArea = area.reduced(padding, 0);

        // Calculate group heights
        int clockTimingHeight = clockTimingGroup->getMinimumHeight();
        int playbackHeight = playbackGroup->getMinimumHeight();
        int midiHeight = midiGroup->getMinimumHeight();
        int group4Height = group4->getMinimumHeight();
        int group5Height = group5->getMinimumHeight();
        int group6Height = group6->getMinimumHeight();

        // 3x2 Grid layout using Grid
        juce::Grid grid;

        using Track = juce::Grid::TrackInfo;
        using Fr = juce::Grid::Fr;
        using Px = juce::Grid::Px;

        // Three equal columns
        grid.templateColumns = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };

        // Two rows with heights based on max height in each row
        int row1Height = juce::jmax(clockTimingHeight, juce::jmax(playbackHeight, midiHeight));
        int row2Height = juce::jmax(group4Height, juce::jmax(group5Height, group6Height));
        grid.templateRows = { Track(Px(row1Height)), Track(Px(row2Height)) };

        // Set gaps
        grid.columnGap = Px(groupHorizontalSpacing);
        grid.rowGap = Px(groupSpacing);

        // Add items in 3x2 grid
        // Row 1
        grid.items.add(juce::GridItem(*clockTimingGroup).withArea(1, 1));
        grid.items.add(juce::GridItem(*playbackGroup).withArea(1, 2));
        grid.items.add(juce::GridItem(*midiGroup).withArea(1, 3));

        // Row 2
        grid.items.add(juce::GridItem(*group4).withArea(2, 1));
        grid.items.add(juce::GridItem(*group5).withArea(2, 2));
        grid.items.add(juce::GridItem(*group6).withArea(2, 3));

        // Perform layout
        grid.performLayout(contentArea);
    }

    // Accessors for navbar
    NavBarComponent& getNavBar() { return navBar; }

    // Callbacks for state changes
    std::function<void(bool)> onInternalClockChanged;
    std::function<void(double)> onTempoChanged;
    std::function<void(double)> onNoteLengthChanged;
    std::function<void(bool)> onPreviewNotesChanged;
    std::function<void(bool)> onFollowStepChanged;
    std::function<void(bool)> onMidiClockOutputChanged;
    std::function<void(bool)> onPianoKeyMomentaryChanged;

    // Accessors for current values
    bool isInternalClockEnabled() const { return internalClockToggle.getToggleState(); }
    double getTempo() const { return tempoSlider.getValue(); }
    double getNoteLength() const { return noteLengthSlider.getValue(); }
    bool isPreviewNotesEnabled() const { return previewNotesToggle.getToggleState(); }
    bool isFollowStepEnabled() const { return followStepToggle.getToggleState(); }
    bool isPianoKeyMomentaryEnabled() const { return pianoKeyMomentaryToggle.getToggleState(); }

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

    void setMidiClockOutputEnabled(bool enabled) {
        midiClockOutputToggle.setToggleState(enabled, juce::dontSendNotification);
    }

    bool isMidiClockOutputEnabled() const {
        return midiClockOutputToggle.getToggleState();
    }

    void setPianoKeyMomentaryEnabled(bool enabled) {
        pianoKeyMomentaryToggle.setToggleState(enabled, juce::dontSendNotification);
    }

private:
    juce::Typeface::Ptr interTypeface;

    NavBarComponent navBar;

    // Settings groups
    std::unique_ptr<SettingsGroup> clockTimingGroup;
    std::unique_ptr<SettingsGroup> playbackGroup;
    std::unique_ptr<SettingsGroup> midiGroup;
    std::unique_ptr<SettingsGroup> group4;
    std::unique_ptr<SettingsGroup> group5;
    std::unique_ptr<SettingsGroup> group6;

    // Clock & Timing controls
    juce::ToggleButton internalClockToggle;
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::ToggleButton midiClockOutputToggle;

    // Playback controls
    juce::Label noteLengthLabel;
    juce::Slider noteLengthSlider;
    juce::ToggleButton previewNotesToggle;
    juce::ToggleButton followStepToggle;

    // Piano Roll controls
    juce::ToggleButton pianoKeyMomentaryToggle;

    // Placeholder labels (TODO: replace with actual controls)
    juce::Label midiPlaceholderLabel;
    juce::Label group4PlaceholderLabel;
    juce::Label group5PlaceholderLabel;
};

