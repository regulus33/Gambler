#pragma once

#include "BinaryData.h"
#include "GlobalScaleQuantizerComponent.h"
#include "NavBarComponent.h"
#include "NoteColourPickerComponent.h"
#include "SettingsGroup.h"
#include "UIConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>

class PluginProcessor;

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
            std::unique_ptr<juce::Component>(selector), getScreenBounds(), nullptr);
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
    explicit SettingsScreenComponent(PluginProcessor& processor)
        : navBar(processor)
    {
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
            bool intEnabled = internalClockToggle.getToggleState();
            tempoSlider.setEnabled(intEnabled);
            if (intEnabled) {
                // Mutually exclusive: disable external MIDI clock
                externalMidiClockToggle.setToggleState(false, juce::dontSendNotification);
            }
            if (onInternalClockChanged)
                onInternalClockChanged(intEnabled);
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

        // Setup External MIDI Clock checkbox
        externalMidiClockToggle.setButtonText("External MIDI Clock");
        externalMidiClockToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        externalMidiClockToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        externalMidiClockToggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
        externalMidiClockToggle.setToggleState(false, juce::dontSendNotification);
        externalMidiClockToggle.onClick = [this] {
            bool extEnabled = externalMidiClockToggle.getToggleState();
            if (extEnabled) {
                // Mutually exclusive: disable internal clock and tempo
                internalClockToggle.setToggleState(false, juce::dontSendNotification);
                internalClockToggle.setEnabled(false);
                tempoSlider.setEnabled(false);
            } else {
                // Re-enable internal clock toggle
                internalClockToggle.setEnabled(true);
                tempoSlider.setEnabled(internalClockToggle.getToggleState());
            }
            if (onExternalMidiClockChanged)
                onExternalMidiClockChanged(extEnabled);
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
        followStepToggle.setToggleState(false, juce::dontSendNotification); // Disabled by default
        followStepToggle.onClick = [this] {
            if (onFollowStepChanged)
                onFollowStepChanged(followStepToggle.getToggleState());
        };



        // Create settings groups
        clockTimingGroup = std::make_unique<SettingsGroup>("Clock & Timing");
        clockTimingGroup->addControl(internalClockToggle);
        clockTimingGroup->addLabeledControl(tempoLabel, tempoSlider);
        clockTimingGroup->addControl(midiClockOutputToggle);
        clockTimingGroup->addControl(externalMidiClockToggle);
        addAndMakeVisible(*clockTimingGroup);

        playbackGroup = std::make_unique<SettingsGroup>("Playback");
        playbackGroup->addLabeledControl(noteLengthLabel, noteLengthSlider);
        playbackGroup->addControl(previewNotesToggle);
        playbackGroup->addControl(followStepToggle);
        addAndMakeVisible(*playbackGroup);

        // Setup MIDI output channel dropdown
        midiOutputChannelLabel.setText("Output Channel", juce::dontSendNotification);
        midiOutputChannelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        midiOutputChannelLabel.setJustificationType(juce::Justification::centredLeft);

        midiOutputChannelComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A2A));
        midiOutputChannelComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
        midiOutputChannelComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
        midiOutputChannelComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::white);
        for (int i = 1; i <= 16; ++i) {
            midiOutputChannelComboBox.addItem(juce::String(i), i);
        }
        midiOutputChannelComboBox.setSelectedId(1, juce::dontSendNotification);
        midiOutputChannelComboBox.onChange = [this] {
            if (onMidiOutputChannelChanged)
                onMidiOutputChannelChanged(midiOutputChannelComboBox.getSelectedId());
        };

        midiGroup = std::make_unique<SettingsGroup>("MIDI");
        midiGroup->addLabeledControl(midiOutputChannelLabel, midiOutputChannelComboBox);
        addAndMakeVisible(*midiGroup);

        // Note Colours group - mini piano keyboard colour picker
        noteColourPicker = std::make_unique<NoteColourPickerComponent>(processor);
        noteColoursGroup = std::make_unique<SettingsGroup>("Note Colours");
        noteColoursGroup->addCustomControl(*noteColourPicker, 62);
        addAndMakeVisible(*noteColoursGroup);

        // Scale Quantizer group - global scale setting for all steps
        globalScaleQuantizer_ = std::make_unique<GlobalScaleQuantizerComponent>(processor);
        scaleQuantizerGroup = std::make_unique<SettingsGroup>("Scale Quantizer");
        scaleQuantizerGroup->addCustomControl(*globalScaleQuantizer_, 100);
        addAndMakeVisible(*scaleQuantizerGroup);

        // Step Colours group
        selectedStepOutlineLabel.setText("Selected step outline", juce::dontSendNotification);
        selectedStepOutlineLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        selectedStepOutlineLabel.setJustificationType(juce::Justification::centredLeft);

        selectedStepSwatch.onColourChanged = [](juce::Colour c) {
            UIConstants::selectedStepOutlineColour = c;
        };

        activeStepLabel.setText("Active step colour", juce::dontSendNotification);
        activeStepLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        activeStepLabel.setJustificationType(juce::Justification::centredLeft);

        activeStepSwatch.onColourChanged = [](juce::Colour c) {
            UIConstants::activeStepColour = c;
        };

        group6 = std::make_unique<SettingsGroup>("Step Colours");
        group6->addLabeledControl(selectedStepOutlineLabel, selectedStepSwatch);
        group6->addLabeledControl(activeStepLabel, activeStepSwatch);
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
        int group4Height = noteColoursGroup->getMinimumHeight();
        int group5Height = scaleQuantizerGroup->getMinimumHeight();
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
        grid.items.add(juce::GridItem(*noteColoursGroup).withArea(2, 1));
        grid.items.add(juce::GridItem(*scaleQuantizerGroup).withArea(2, 2));
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
    std::function<void(bool)> onExternalMidiClockChanged;
    std::function<void(int)> onMidiOutputChannelChanged;

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

    void setMidiClockOutputEnabled(bool enabled) {
        midiClockOutputToggle.setToggleState(enabled, juce::dontSendNotification);
    }

    bool isMidiClockOutputEnabled() const {
        return midiClockOutputToggle.getToggleState();
    }

    void setExternalMidiClockEnabled(bool enabled) {
        externalMidiClockToggle.setToggleState(enabled, juce::dontSendNotification);
        if (enabled) {
            internalClockToggle.setEnabled(false);
            tempoSlider.setEnabled(false);
        } else {
            internalClockToggle.setEnabled(true);
            tempoSlider.setEnabled(internalClockToggle.getToggleState());
        }
    }

    bool isExternalMidiClockEnabled() const {
        return externalMidiClockToggle.getToggleState();
    }

    void setMidiOutputChannel(int channel) {
        midiOutputChannelComboBox.setSelectedId(channel, juce::dontSendNotification);
    }

    int getMidiOutputChannel() const {
        return midiOutputChannelComboBox.getSelectedId();
    }

private:
    juce::Typeface::Ptr interTypeface;

    NavBarComponent navBar;

    // Settings groups
    std::unique_ptr<SettingsGroup> clockTimingGroup;
    std::unique_ptr<SettingsGroup> playbackGroup;
    std::unique_ptr<SettingsGroup> midiGroup;
    std::unique_ptr<SettingsGroup> noteColoursGroup;
    std::unique_ptr<SettingsGroup> scaleQuantizerGroup;
    std::unique_ptr<SettingsGroup> group6;

    // Clock & Timing controls
    juce::ToggleButton internalClockToggle;
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::ToggleButton midiClockOutputToggle;
    juce::ToggleButton externalMidiClockToggle;

    // Playback controls
    juce::Label noteLengthLabel;
    juce::Slider noteLengthSlider;
    juce::ToggleButton previewNotesToggle;
    juce::ToggleButton followStepToggle;

    // MIDI controls
    juce::Label midiOutputChannelLabel;
    juce::ComboBox midiOutputChannelComboBox;

    // Note colour picker
    std::unique_ptr<NoteColourPickerComponent> noteColourPicker;

    // Global scale quantizer
    std::unique_ptr<GlobalScaleQuantizerComponent> globalScaleQuantizer_;

    // Step colour controls
    juce::Label selectedStepOutlineLabel;
    ColourSwatchComponent selectedStepSwatch { UIConstants::selectedStepOutlineColour };
    juce::Label activeStepLabel;
    ColourSwatchComponent activeStepSwatch { UIConstants::activeStepColour };
};

