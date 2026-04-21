#pragma once

#include "NavBarConstants.h"
#include "ExportButton.h"
#include "../ToggleIconButton.h"
#include "../ScaleLatchButton.h"
#include "../DestructiveToggleButton.h"
#include "../sliders/SubdivisionSlider.h"
#include "../../data/Params.h"
#include "../../PluginProcessor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * NavBarComponent
 *
 * A navigation bar component shown at the top of all screens.
 */
class NavBarComponent : public juce::Component {
public:
    explicit NavBarComponent(PluginProcessor& processor)
        : processorRef(processor),
          settingsButton(Icons::createSettingsIcon, Icons::createCloseIcon, true, false, NavBarConstants::settingsIconScale, Icons::closeIconScale),
          midiLearnButton(Icons::createMidiLearnIcon, Icons::createMidiLearnIcon, false, true),
          playPauseButton(Icons::createPlayIcon, Icons::createPauseIcon, false, false),
          stopButton(Icons::createStopIcon, Icons::createStopIcon, false, false),
          scaleLatchButton(processor),
          bakeScaleButton(processor, Params::Keys::ID::PREF_DESTRUCTIVE_SCALE,
                          MidiControlId::DESTRUCTIVE_SCALE, Icons::createBakeScaleIcon),
          bakeRangeButton(processor, Params::Keys::ID::PREF_DESTRUCTIVE_RANGE,
                          MidiControlId::DESTRUCTIVE_RANGE, Icons::createBakeRangeIcon),
          compactTopButton(Icons::createCompactTopIcon, Icons::createCompactTopIcon, true, true),
          compactBottomButton(Icons::createCompactViewIcon, Icons::createCompactViewIcon, true, true)
    {
        setupMidiLearnButton();
        setupSettingsButton();
        setupTransportButtons();
        setupScaleLatchButton();
        setupDestructiveButtons();
        setupExportButton();
        setupCompactButtons();
        setupSubdivisionSlider();
    }

    void resized() override {
        using namespace NavBarConstants;

        auto area = getLocalBounds();

        auto addItem = [&](juce::FlexBox& fb, juce::Component& comp, float w, float h, float gap = 0.0f) {
            fb.items.add(juce::FlexItem(comp).withWidth(w).withHeight(h).withMargin({0, 0, 0, gap}));
        };

        juce::FlexBox fb;
        fb.flexDirection = juce::FlexBox::Direction::row;
        fb.alignItems = juce::FlexBox::AlignItems::center;
        fb.justifyContent = juce::FlexBox::JustifyContent::spaceAround;

        auto sz = [](float scale) { return baseButtonSize * scale; };
        float ss = baseSubdivSize * subdivisionScale;

        addItem(fb, compactTopButton,    sz(compactTopScale),    sz(compactTopScale));
        addItem(fb, compactBottomButton, sz(compactBottomScale), sz(compactBottomScale));
        addItem(fb, subdivisionSlider,   ss, ss);
        addItem(fb, exportButton,        sz(exportScale),        sz(exportScale));
        addItem(fb, scaleLatchButton,    sz(scaleLatchScale),    sz(scaleLatchScale));
        addItem(fb, bakeScaleButton,     sz(bakeScaleScale),     sz(bakeScaleScale));
        addItem(fb, bakeRangeButton,     sz(bakeRangeScale),     sz(bakeRangeScale));
        addItem(fb, playPauseButton,     sz(playPauseScale),     sz(playPauseScale));
        addItem(fb, stopButton,          sz(stopScale),          sz(stopScale));
        addItem(fb, midiLearnButton,     sz(midiLearnScale),     sz(midiLearnScale));
        addItem(fb, settingsButton,      sz(settingsScale),      sz(settingsScale));

        fb.performLayout(area.toFloat());
    }

    std::function<void()> onSettingsClicked;
    std::function<void(bool)> onMidiLearnClicked;
    std::function<void()> onPlayPauseClicked;
    std::function<void()> onStopClicked;
    std::function<void()> onExportClicked;
    std::function<juce::File()> onExportDragStarted;
    std::function<void(bool)> onCompactViewToggled;
    std::function<void(bool)> onSwapRowClicked;

    void enterCompactBottomMode() {
        if (!compactBottomButton.getToggled()) {
            compactBottomButton.setToggled(true);
            compactTopButton.setToggled(false);
            compactTopButton.stopPulse();
            compactBottomButton.startPulse(NavBarConstants::compactPulseColor);
            if (onCompactViewToggled) onCompactViewToggled(true);
            if (onSwapRowClicked) onSwapRowClicked(false);
        }
    }

    juce::Rectangle<int> getExportButtonScreenBounds() const {
        return exportButton.localAreaToGlobal(exportButton.getLocalBounds());
    }

    juce::Component* getExportButton() { return &exportButton; }

    void setSettingsToggled(bool toggled) { settingsButton.setToggled(toggled); }
    void setMidiLearnToggled(bool toggled) { midiLearnButton.setToggled(toggled); }
    void setPlayPauseState(bool isPlaying) { playPauseButton.setToggled(isPlaying); }

    void setStopButtonActive(bool isStopped) {
        stopButton.setColor1(isStopped ? NavBarConstants::activeStopColor : NavBarConstants::inactiveColor);
    }

    SubdivisionSlider& getSubdivisionSlider() { return subdivisionSlider; }
    DestructiveToggleButton& getBakeScaleButton() { return bakeScaleButton; }
    DestructiveToggleButton& getBakeRangeButton() { return bakeRangeButton; }

    void setSettingsButtonRegistry(MidiLearnRegistry* registry) {
        settingsButton.setRegistry(registry);
        playPauseButton.setRegistry(registry);
        stopButton.setRegistry(registry);
        scaleLatchButton.setRegistry(registry);
        bakeScaleButton.setRegistry(registry);
        bakeRangeButton.setRegistry(registry);
    }

private:
    void setupMidiLearnButton() {
        addAndMakeVisible(midiLearnButton);
        midiLearnButton.onClick = [this](bool toggled) {
            if (onMidiLearnClicked) onMidiLearnClicked(toggled);
        };
        midiLearnButton.setColor1(NavBarConstants::inactiveColor);
        midiLearnButton.setColor2(juce::Colours::grey);
    }

    void setupSettingsButton() {
        addAndMakeVisible(settingsButton);
        settingsButton.setMidiControlId(MidiControlId::SETTINGS_TOGGLE);
        settingsButton.onClick = [this](bool /*toggled*/) {
            if (onSettingsClicked) onSettingsClicked();
        };
    }

    void setupTransportButtons() {
        addAndMakeVisible(playPauseButton);
        playPauseButton.setMidiControlId(MidiControlId::PLAY_PAUSE);
        playPauseButton.onClick = [this](bool /*toggled*/) {
            if (onPlayPauseClicked) onPlayPauseClicked();
        };
        playPauseButton.setColor1(NavBarConstants::inactiveColor);
        playPauseButton.setColor2(NavBarConstants::activePlayColor);
        playPauseButton.setFlashEnabled(false);

        addAndMakeVisible(stopButton);
        stopButton.setMidiControlId(MidiControlId::STOP);
        stopButton.onClick = [this](bool /*toggled*/) {
            if (onStopClicked) onStopClicked();
        };
        stopButton.setColor1(NavBarConstants::inactiveColor);
        stopButton.setColor2(NavBarConstants::inactiveColor);
        stopButton.setFlashEnabled(false);
    }

    void setupScaleLatchButton() {
        addAndMakeVisible(scaleLatchButton);
    }

    void setupDestructiveButtons() {
        addAndMakeVisible(bakeScaleButton);
        addAndMakeVisible(bakeRangeButton);
    }

    void setupExportButton() {
        addAndMakeVisible(exportButton);
        exportButton.onClicked = [this]() {
            if (onExportClicked) onExportClicked();
        };
        exportButton.onDragStarted = [this]() -> juce::File {
            if (onExportDragStarted) return onExportDragStarted();
            return {};
        };
    }

    void setupCompactButtons() {
        addAndMakeVisible(compactTopButton);
        compactTopButton.onClick = [this](bool /*toggled*/) {
            if (compactTopButton.getToggled()) {
                compactBottomButton.setToggled(false);
                compactBottomButton.stopPulse();
                compactTopButton.startPulse(NavBarConstants::compactPulseColor);
                if (onCompactViewToggled) onCompactViewToggled(true);
                if (onSwapRowClicked) onSwapRowClicked(true);
            } else {
                compactTopButton.stopPulse();
                if (onCompactViewToggled) onCompactViewToggled(false);
            }
        };

        addAndMakeVisible(compactBottomButton);
        compactBottomButton.onClick = [this](bool /*toggled*/) {
            if (compactBottomButton.getToggled()) {
                compactTopButton.setToggled(false);
                compactTopButton.stopPulse();
                compactBottomButton.startPulse(NavBarConstants::compactPulseColor);
                if (onCompactViewToggled) onCompactViewToggled(true);
                if (onSwapRowClicked) onSwapRowClicked(false);
            } else {
                compactBottomButton.stopPulse();
                if (onCompactViewToggled) onCompactViewToggled(false);
            }
        };
    }

    void setupSubdivisionSlider() {
        addAndMakeVisible(subdivisionSlider);
        subdivisionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processorRef.getAPVTS(), Params::Keys::ID::CLOCK_SUBDIVISION,
            subdivisionSlider.getSlider());
    }

    PluginProcessor& processorRef;
    ToggleIconButton settingsButton;
    ToggleIconButton midiLearnButton;
    ToggleIconButton playPauseButton;
    ToggleIconButton stopButton;
    ScaleLatchButton scaleLatchButton;
    DestructiveToggleButton bakeScaleButton;
    DestructiveToggleButton bakeRangeButton;
    ToggleIconButton compactTopButton;
    ToggleIconButton compactBottomButton;
    ExportButton exportButton;
    SubdivisionSlider subdivisionSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subdivisionAttachment;
};
