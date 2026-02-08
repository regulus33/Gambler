#pragma once

#include "BinaryData.h"
#include "LogoComponent.h"
#include "PresetSelectorComponent.h"
#include "ToggleIconButton.h"
#include "WriteScaleButton.h"
#include "ScaleLatchButton.h"
#include "ScaleClearButton.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class PluginProcessor;

namespace NavBarConstants {
    constexpr int height = 45;  // Height of the navbar
    constexpr int logoGapFromTop = 10;  // Gap between top and logo
    constexpr int presetSelectorGapFromLogo = 20;  // Gap between logo and preset selector
    constexpr int versionLabelWidth = 60;
    constexpr int versionLabelHeight = 20;
    constexpr int versionLabelMarginRight = 10;  // Gap between version and view toggle button
    constexpr int buttonSize = ToggleIconButtonConstants::totalSize;  // 36px
    constexpr int buttonGap = 10;  // Gap between buttons
    constexpr int settingsButtonOffsetRight = 46;  // Offset from right edge

    // Transport button colors
    const juce::Colour inactiveColor = juce::Colours::white;
    const juce::Colour activePlayColor = juce::Colour(0xff4ade80);  // Green-400 for playing state
    const juce::Colour activeStopColor = juce::Colour(0xfff87171);  // Red-400 for stopped state
}

/**
 * NavBarComponent
 *
 * A navigation bar component that displays:
 * - Logo (left side, centered vertically)
 * - Preset selector (right of logo)
 * - Version label (right side, left of view toggle button)
 * - View toggle button (right side, left of settings button)
 * - Settings button (right side)
 *
 * This component is displayed at the top of all screens.
 */
class NavBarComponent : public juce::Component {
public:
    explicit NavBarComponent(PluginProcessor& processor)
        : viewToggleButton(Icons::createViewToggleIcon1, Icons::createViewToggleIcon2, false, true),
          settingsButton(Icons::createSettingsIcon, Icons::createCloseIcon, true, false, 0.75f, Icons::closeIconScale),
          midiLearnButton(Icons::createMidiLearnIcon, Icons::createMidiLearnIcon, true, true),
          playPauseButton(Icons::createPlayIcon, Icons::createPauseIcon, false, false),
          stopButton(Icons::createStopIcon, Icons::createStopIcon, false, false),
          writeScaleButton(processor),
          scaleLatchButton(processor),
          scaleClearButton(processor)
    {
        // Add logo component
        addAndMakeVisible(logo);

        // Add preset selector
        addAndMakeVisible(presetSelector);

        // Add version label
        addAndMakeVisible(versionLabel);
        versionLabel.setText("v" VERSION, juce::dontSendNotification);
        auto interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_28ptBold_ttf,
            BinaryData::Inter_28ptBold_ttfSize);
        versionLabel.setFont(juce::FontOptions(interBoldFont).withHeight(11.0f));
        versionLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        versionLabel.setJustificationType(juce::Justification::centredRight);

        // Add MIDI learn button
        addAndMakeVisible(midiLearnButton);
        midiLearnButton.onClick = [this](bool toggled) {
            if (onMidiLearnClicked) {
                onMidiLearnClicked(toggled);
            }
        };

        // Add view toggle button
        addAndMakeVisible(viewToggleButton);
        viewToggleButton.onClick = [this](bool toggled) {
            if (onViewToggleClicked) {
                onViewToggleClicked(toggled);
            }
        };

        // Add settings button
        addAndMakeVisible(settingsButton);
        settingsButton.onClick = [this](bool toggled) {
            if (onSettingsClicked) {
                onSettingsClicked();
            }
        };

        // Add play/pause button
        addAndMakeVisible(playPauseButton);
        playPauseButton.onClick = [this](bool toggled) {
            if (onPlayPauseClicked) {
                onPlayPauseClicked();
            }
        };

        // Add stop button
        addAndMakeVisible(stopButton);
        stopButton.onClick = [this](bool toggled) {
            if (onStopClicked) {
                onStopClicked();
            }
        };

        // Add write scale button
        addAndMakeVisible(writeScaleButton);
        // Note: No callback needed - logic is handled in audio thread

        // Add scale latch button
        addAndMakeVisible(scaleLatchButton);
        // Note: No callback needed - syncs with APVTS parameter

        // Add scale clear button
        addAndMakeVisible(scaleClearButton);
        // Note: No callback needed - triggers clear action via APVTS parameter

        // Set MIDI learn button colors and stroke width
        // White when OFF (state 1), grey when ON/active (state 2)
        midiLearnButton.setColor1(NavBarConstants::inactiveColor);
        midiLearnButton.setColor2(juce::Colours::grey);

        // Set transport button colors
        // Play/Pause: white when stopped/paused (state 1), green when playing (state 2)
        playPauseButton.setColor1(NavBarConstants::inactiveColor);
        playPauseButton.setColor2(NavBarConstants::activePlayColor);

        // Stop: white normally (we'll update this dynamically based on state)
        stopButton.setColor1(NavBarConstants::inactiveColor);
        stopButton.setColor2(NavBarConstants::inactiveColor);
    }

    void paint(juce::Graphics& g) override {
        // Navbar has transparent background - parent will paint the background
    }

    void resized() override {
        using namespace NavBarConstants;

        auto area = getLocalBounds();

        // Position logo on the left, centered vertically
        int logoWidth = logo.getCalculatedWidth();
        int logoHeight = logo.getCalculatedHeight();
        int logoX = 22;  // Match left margin from main layout
        int logoY = (height - logoHeight) / 2;
        logo.setBounds(logoX, logoY, logoWidth, logoHeight);

        // Position preset selector to the right of logo
        int presetSelectorWidth = presetSelector.getCalculatedWidth();
        int presetSelectorHeight = presetSelector.getCalculatedHeight();
        int presetSelectorX = logoX + logoWidth + presetSelectorGapFromLogo;
        int presetSelectorY = (height - presetSelectorHeight) / 2;
        presetSelector.setBounds(presetSelectorX, presetSelectorY, presetSelectorWidth, presetSelectorHeight);

        // Position settings button in top right corner
        int settingsButtonX = getWidth() - buttonSize - settingsButtonOffsetRight;
        int settingsButtonY = (height - buttonSize) / 2;  // Vertically center
        settingsButton.setBounds(settingsButtonX, settingsButtonY, buttonSize, buttonSize);

        // Position view toggle button left of settings button
        int viewToggleButtonX = settingsButtonX - buttonSize - buttonGap;
        int viewToggleButtonY = (height - buttonSize) / 2;  // Vertically center
        viewToggleButton.setBounds(viewToggleButtonX, viewToggleButtonY, buttonSize, buttonSize);

        // Position MIDI learn button left of view toggle button
        int midiLearnButtonX = viewToggleButtonX - buttonSize - buttonGap;
        int midiLearnButtonY = (height - buttonSize) / 2;  // Vertically center
        midiLearnButton.setBounds(midiLearnButtonX, midiLearnButtonY, buttonSize, buttonSize);

        // Position version label left of MIDI learn button
        int versionLabelX = midiLearnButtonX - versionLabelWidth - versionLabelMarginRight;
        int versionLabelY = (height - versionLabelHeight) / 2;  // Vertically center
        versionLabel.setBounds(versionLabelX, versionLabelY, versionLabelWidth, versionLabelHeight);

        // Position stop button left of version label
        int stopButtonX = versionLabelX - buttonSize - buttonGap;
        int stopButtonY = (height - buttonSize) / 2;  // Vertically center
        stopButton.setBounds(stopButtonX, stopButtonY, buttonSize, buttonSize);

        // Position play/pause button left of stop button
        int playPauseButtonX = stopButtonX - buttonSize - buttonGap;
        int playPauseButtonY = (height - buttonSize) / 2;  // Vertically center
        playPauseButton.setBounds(playPauseButtonX, playPauseButtonY, buttonSize, buttonSize);

        // Position write scale button left of play/pause button
        int writeScaleButtonX = playPauseButtonX - buttonSize - buttonGap;
        int writeScaleButtonY = (height - buttonSize) / 2;  // Vertically center
        writeScaleButton.setBounds(writeScaleButtonX, writeScaleButtonY, buttonSize, buttonSize);

        // Position scale latch button left of write scale button
        int scaleLatchButtonX = writeScaleButtonX - buttonSize - buttonGap;
        int scaleLatchButtonY = (height - buttonSize) / 2;  // Vertically center
        scaleLatchButton.setBounds(scaleLatchButtonX, scaleLatchButtonY, buttonSize, buttonSize);

        // Position scale clear button left of scale latch button
        int scaleClearButtonX = scaleLatchButtonX - buttonSize - buttonGap;
        int scaleClearButtonY = (height - buttonSize) / 2;  // Vertically center
        scaleClearButton.setBounds(scaleClearButtonX, scaleClearButtonY, buttonSize, buttonSize);
    }

    // Callback when settings button is clicked
    std::function<void()> onSettingsClicked;

    // Callback when view toggle button is clicked
    // Parameter: true = toggled on (icon 2), false = toggled off (icon 1)
    std::function<void(bool)> onViewToggleClicked;

    // Callback when MIDI learn button is clicked
    // Parameter: true = MIDI learn mode ON, false = MIDI learn mode OFF
    std::function<void(bool)> onMidiLearnClicked;

    // Callback when play/pause button is clicked
    std::function<void()> onPlayPauseClicked;

    // Callback when stop button is clicked
    std::function<void()> onStopClicked;

    // Note: WriteScale button doesn't need a callback - MainScreenComponent checks state directly

    // Public method to set settings button toggle state (for ApplicationComponent)
    void setSettingsToggled(bool toggled) {
        settingsButton.setToggled(toggled);
    }

    // Public method to set MIDI learn button toggle state (for ApplicationComponent)
    void setMidiLearnToggled(bool toggled) {
        midiLearnButton.setToggled(toggled);
    }

    // Public method to set play/pause button state based on sequencer state
    // When playing: shows pause icon (toggled = true) with green color
    // When stopped/paused: shows play icon (toggled = false) with white color
    void setPlayPauseState(bool isPlaying) {
        playPauseButton.setToggled(isPlaying);
    }

    // Public method to set stop button color based on sequencer state
    // When stopped: shows red color
    // When playing/paused: shows white color
    void setStopButtonActive(bool isStopped) {
        if (isStopped) {
            stopButton.setColor1(NavBarConstants::activeStopColor);
        } else {
            stopButton.setColor1(NavBarConstants::inactiveColor);
        }
    }

    // Access to preset selector for wiring up callbacks
    PresetSelectorComponent& getPresetSelector() { return presetSelector; }

    // Access to write scale button for checking state
    WriteScaleButton& getWriteScaleButton() { return writeScaleButton; }

private:
    LogoComponent logo;
    PresetSelectorComponent presetSelector;
    juce::Label versionLabel;
    ToggleIconButton midiLearnButton;
    ToggleIconButton viewToggleButton;
    ToggleIconButton settingsButton;
    ToggleIconButton playPauseButton;
    ToggleIconButton stopButton;
    WriteScaleButton writeScaleButton;
    ScaleLatchButton scaleLatchButton;
    ScaleClearButton scaleClearButton;
};

