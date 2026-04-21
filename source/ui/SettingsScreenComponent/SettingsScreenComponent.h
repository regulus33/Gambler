#pragma once

#include "ColourSwatchComponent.h"
#include "SettingsScreenConstants.h"
#include "BinaryData.h"
#include "../GlobalScaleQuantizerComponent.h"
#include "../NavBarComponent/NavBarComponent.h"
#include "../NoteColourPickerComponent.h"
#include "../SettingsGroup/SettingsGroup.h"
#include "../UIConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <functional>

class PluginProcessor;

/**
 * SettingsScreenComponent
 *
 * A full-screen settings view with:
 * - Clock & Timing (internal clock, tempo, MIDI clock out, external MIDI clock)
 * - Scale Quantizer
 * - Playback & MIDI (note length, preview notes, follow step, MIDI output channel)
 * - Colours (note colour picker, step colour swatches)
 *
 * Laid out as a 2×2 grid below the NavBar (NavBar is owned by ApplicationComponent).
 */
class SettingsScreenComponent : public juce::Component {
public:
    explicit SettingsScreenComponent(PluginProcessor& processor) {
        interTypeface = juce::Typeface::createSystemTypefaceFor(
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        setupClockControls();
        setupPlaybackControls();
        setupMidiChannelControl();
        setupColourControls();
        buildGroups(processor);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(SettingsScreenConstants::bgColourHex));
    }

    void resized() override {
        using namespace SettingsScreenConstants;

        auto area = getLocalBounds();
        area.removeFromTop(NavBarConstants::height);
        area.removeFromTop(28);

        auto contentArea = area.reduced(padding, 0);

        juce::Grid grid;
        using Track = juce::Grid::TrackInfo;
        using Fr = juce::Grid::Fr;
        using Px = juce::Grid::Px;

        grid.templateColumns = { Track(Fr(1)), Track(Fr(1)) };

        int row1Height = juce::jmax(clockTimingGroup->getMinimumHeight(),
                                    scaleQuantizerGroup->getMinimumHeight());
        int row2Height = juce::jmax(playbackMidiGroup->getMinimumHeight(),
                                    coloursGroup->getMinimumHeight());
        grid.templateRows = { Track(Px(row1Height)), Track(Px(row2Height)) };
        grid.columnGap = Px(groupHorizontalSpacing);
        grid.rowGap = Px(groupSpacing);

        grid.items.add(juce::GridItem(*clockTimingGroup).withArea(1, 1));
        grid.items.add(juce::GridItem(*scaleQuantizerGroup).withArea(1, 2));
        grid.items.add(juce::GridItem(*playbackMidiGroup).withArea(2, 1));
        grid.items.add(juce::GridItem(*coloursGroup).withArea(2, 2));

        grid.performLayout(contentArea);
    }

    std::function<void(bool)> onInternalClockChanged;
    std::function<void(double)> onTempoChanged;
    std::function<void(double)> onNoteLengthChanged;
    std::function<void(bool)> onPreviewNotesChanged;
    std::function<void(bool)> onFollowStepChanged;
    std::function<void(bool)> onMidiClockOutputChanged;
    std::function<void(bool)> onExternalMidiClockChanged;
    std::function<void(int)> onMidiOutputChannelChanged;

    bool isInternalClockEnabled() const { return internalClockToggle.getToggleState(); }
    double getTempo() const { return tempoSlider.getValue(); }
    double getNoteLength() const { return noteLengthSlider.getValue(); }
    bool isPreviewNotesEnabled() const { return previewNotesToggle.getToggleState(); }
    bool isFollowStepEnabled() const { return followStepToggle.getToggleState(); }

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

    bool isMidiClockOutputEnabled() const { return midiClockOutputToggle.getToggleState(); }

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

    bool isExternalMidiClockEnabled() const { return externalMidiClockToggle.getToggleState(); }

    void setMidiOutputChannel(int channel) {
        midiOutputChannelComboBox.setSelectedId(channel, juce::dontSendNotification);
    }
    int getMidiOutputChannel() const { return midiOutputChannelComboBox.getSelectedId(); }

private:
    void styleToggle(juce::ToggleButton& toggle, const juce::String& label) {
        toggle.setButtonText(label);
        toggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        toggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::white);
        toggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::grey);
    }

    void styleLabel(juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setJustificationType(juce::Justification::centredLeft);
    }

    void styleSlider(juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false,
                               static_cast<int>(SettingsScreenConstants::sliderTextBoxWidth),
                               SettingsScreenConstants::controlHeight);
        slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        slider.setColour(juce::Slider::trackColourId, juce::Colours::grey);
    }

    void setupClockControls() {
        styleToggle(internalClockToggle, "Internal clock");
        internalClockToggle.onClick = [this] {
            bool intEnabled = internalClockToggle.getToggleState();
            tempoSlider.setEnabled(intEnabled);
            if (intEnabled)
                externalMidiClockToggle.setToggleState(false, juce::dontSendNotification);
            if (onInternalClockChanged) onInternalClockChanged(intEnabled);
        };

        styleSlider(tempoSlider);
        tempoSlider.setRange(1.0, 1000.0, 1.0);
        tempoSlider.setValue(120.0);
        tempoSlider.setTextValueSuffix(" BPM");
        tempoSlider.setEnabled(false);
        tempoSlider.onValueChange = [this] {
            if (onTempoChanged) onTempoChanged(tempoSlider.getValue());
        };

        styleLabel(tempoLabel, "Tempo");

        styleToggle(midiClockOutputToggle, "MIDI Clock Output");
        midiClockOutputToggle.setToggleState(false, juce::dontSendNotification);
        midiClockOutputToggle.onClick = [this] {
            if (onMidiClockOutputChanged)
                onMidiClockOutputChanged(midiClockOutputToggle.getToggleState());
        };

        styleToggle(externalMidiClockToggle, "External MIDI Clock");
        externalMidiClockToggle.setToggleState(false, juce::dontSendNotification);
        externalMidiClockToggle.onClick = [this] {
            bool extEnabled = externalMidiClockToggle.getToggleState();
            if (extEnabled) {
                internalClockToggle.setToggleState(false, juce::dontSendNotification);
                internalClockToggle.setEnabled(false);
                tempoSlider.setEnabled(false);
            } else {
                internalClockToggle.setEnabled(true);
                tempoSlider.setEnabled(internalClockToggle.getToggleState());
            }
            if (onExternalMidiClockChanged) onExternalMidiClockChanged(extEnabled);
        };
    }

    void setupPlaybackControls() {
        styleSlider(noteLengthSlider);
        noteLengthSlider.setRange(0.1, 1.0, 0.01);
        noteLengthSlider.setValue(0.9);
        noteLengthSlider.setTextValueSuffix(" %");
        noteLengthSlider.onValueChange = [this] {
            if (onNoteLengthChanged) onNoteLengthChanged(noteLengthSlider.getValue());
        };
        noteLengthSlider.textFromValueFunction = [](double value) {
            return juce::String(static_cast<int>(value * 100.0));
        };
        noteLengthSlider.valueFromTextFunction = [](const juce::String& text) {
            return text.getDoubleValue() / 100.0;
        };

        styleLabel(noteLengthLabel, "Note Length");

        styleToggle(previewNotesToggle, "Preview notes on edit");
        previewNotesToggle.setToggleState(true, juce::dontSendNotification);
        previewNotesToggle.onClick = [this] {
            if (onPreviewNotesChanged) onPreviewNotesChanged(previewNotesToggle.getToggleState());
        };

        styleToggle(followStepToggle, "Follow step");
        followStepToggle.setToggleState(false, juce::dontSendNotification);
        followStepToggle.onClick = [this] {
            if (onFollowStepChanged) onFollowStepChanged(followStepToggle.getToggleState());
        };
    }

    void setupMidiChannelControl() {
        styleLabel(midiOutputChannelLabel, "MIDI Channel");

        juce::StringArray channelNames;
        for (int i = 1; i <= 16; ++i)
            channelNames.add(juce::String(i));
        midiOutputChannelComboBox.setItems(channelNames);
        midiOutputChannelComboBox.setSelectedId(1, juce::dontSendNotification);
        midiOutputChannelComboBox.onChange = [this] {
            if (onMidiOutputChannelChanged)
                onMidiOutputChannelChanged(midiOutputChannelComboBox.getSelectedId());
        };
    }

    void setupColourControls() {
        styleLabel(selectedStepOutlineLabel, "Selected step outline");
        selectedStepSwatch.onColourChanged = [](juce::Colour c) {
            UIConstants::selectedStepOutlineColour = c;
        };

        styleLabel(activeStepLabel, "Active step colour");
        activeStepSwatch.onColourChanged = [](juce::Colour c) {
            UIConstants::activeStepColour = c;
        };
    }

    void buildGroups(PluginProcessor& processor) {
        clockTimingGroup = std::make_unique<SettingsGroup>("Clock & Timing");
        clockTimingGroup->addControl(internalClockToggle);
        clockTimingGroup->addLabeledControl(tempoLabel, tempoSlider);
        clockTimingGroup->addControl(midiClockOutputToggle);
        clockTimingGroup->addControl(externalMidiClockToggle);
        addAndMakeVisible(*clockTimingGroup);

        globalScaleQuantizer_ = std::make_unique<GlobalScaleQuantizerComponent>(processor);
        scaleQuantizerGroup = std::make_unique<SettingsGroup>("Scale Quantizer");
        scaleQuantizerGroup->addCustomControl(*globalScaleQuantizer_,
            GlobalScaleQuantizerConstants::totalHeight);
        addAndMakeVisible(*scaleQuantizerGroup);

        playbackMidiGroup = std::make_unique<SettingsGroup>("Playback & MIDI");
        playbackMidiGroup->addLabeledControl(noteLengthLabel, noteLengthSlider);
        playbackMidiGroup->addControl(previewNotesToggle);
        playbackMidiGroup->addControl(followStepToggle);
        playbackMidiGroup->addLabeledControl(midiOutputChannelLabel, midiOutputChannelComboBox);
        addAndMakeVisible(*playbackMidiGroup);

        noteColourPicker = std::make_unique<NoteColourPickerComponent>(processor);
        coloursGroup = std::make_unique<SettingsGroup>("Colours");
        coloursGroup->addCustomControl(*noteColourPicker, 62);
        coloursGroup->addLabeledControl(selectedStepOutlineLabel, selectedStepSwatch);
        coloursGroup->addLabeledControl(activeStepLabel, activeStepSwatch);
        addAndMakeVisible(*coloursGroup);
    }

    juce::Typeface::Ptr interTypeface;

    std::unique_ptr<SettingsGroup> clockTimingGroup;
    std::unique_ptr<SettingsGroup> scaleQuantizerGroup;
    std::unique_ptr<SettingsGroup> playbackMidiGroup;
    std::unique_ptr<SettingsGroup> coloursGroup;

    juce::ToggleButton internalClockToggle;
    juce::Label tempoLabel;
    juce::Slider tempoSlider;
    juce::ToggleButton midiClockOutputToggle;
    juce::ToggleButton externalMidiClockToggle;

    juce::Label noteLengthLabel;
    juce::Slider noteLengthSlider;
    juce::ToggleButton previewNotesToggle;
    juce::ToggleButton followStepToggle;
    juce::Label midiOutputChannelLabel;
    StyledDropdown midiOutputChannelComboBox;

    std::unique_ptr<NoteColourPickerComponent> noteColourPicker;
    std::unique_ptr<GlobalScaleQuantizerComponent> globalScaleQuantizer_;

    juce::Label selectedStepOutlineLabel;
    ColourSwatchComponent selectedStepSwatch { UIConstants::selectedStepOutlineColour };
    juce::Label activeStepLabel;
    ColourSwatchComponent activeStepSwatch { UIConstants::activeStepColour };
};
