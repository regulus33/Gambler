#pragma once

#include "BinaryData.h"
#include "LogoComponent.h"
#include "PresetSelectorComponent.h"
#include "ToggleIconButton.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace NavBarConstants {
    constexpr int height = 60;  // Height of the navbar
    constexpr int logoGapFromTop = 10;  // Gap between top and logo
    constexpr int presetSelectorGapFromLogo = 20;  // Gap between logo and preset selector
    constexpr int versionLabelWidth = 60;
    constexpr int versionLabelHeight = 20;
    constexpr int versionLabelMarginRight = 10;  // Gap between version and view toggle button
    constexpr int buttonSize = ToggleIconButtonConstants::totalSize;  // 36px
    constexpr int buttonGap = 10;  // Gap between buttons
    constexpr int settingsButtonOffsetRight = 46;  // Offset from right edge
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
    NavBarComponent()
        : viewToggleButton(Icons::createViewToggleIcon1, Icons::createViewToggleIcon2, false, true),
          settingsButton(Icons::createSettingsIcon, Icons::createCloseIcon, true, false),
          midiLearnButton(Icons::createMidiLearnIcon, Icons::createMidiLearnIcon, true, false)
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
    }

    // Callback when settings button is clicked
    std::function<void()> onSettingsClicked;

    // Callback when view toggle button is clicked
    // Parameter: true = toggled on (icon 2), false = toggled off (icon 1)
    std::function<void(bool)> onViewToggleClicked;

    // Callback when MIDI learn button is clicked
    // Parameter: true = MIDI learn mode ON, false = MIDI learn mode OFF
    std::function<void(bool)> onMidiLearnClicked;

    // Public method to set settings button toggle state (for ApplicationComponent)
    void setSettingsToggled(bool toggled) {
        settingsButton.setToggled(toggled);
    }

    // Public method to set MIDI learn button toggle state (for ApplicationComponent)
    void setMidiLearnToggled(bool toggled) {
        midiLearnButton.setToggled(toggled);
    }

    // Access to preset selector for wiring up callbacks
    PresetSelectorComponent& getPresetSelector() { return presetSelector; }

private:
    LogoComponent logo;
    PresetSelectorComponent presetSelector;
    juce::Label versionLabel;
    ToggleIconButton midiLearnButton;
    ToggleIconButton viewToggleButton;
    ToggleIconButton settingsButton;
};

