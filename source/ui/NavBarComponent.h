#pragma once

#include "BinaryData.h"
#include "LogoComponent.h"
#include "ToggleIconButton.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace NavBarConstants {
    constexpr int height = 60;  // Height of the navbar
    constexpr int logoGapFromTop = 10;  // Gap between top and logo
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
 * - Version label (right side, left of view toggle button)
 * - View toggle button (right side, left of settings button)
 * - Settings button (right side)
 *
 * This component is displayed at the top of all screens.
 */
class NavBarComponent : public juce::Component {
public:
    NavBarComponent()
        : viewToggleButton(Icons::createViewToggleIcon1, Icons::createViewToggleIcon2, false),
          settingsButton(Icons::createSettingsIcon, Icons::createCloseIcon, true)
    {
        // Add logo component
        addAndMakeVisible(logo);

        // Add version label
        addAndMakeVisible(versionLabel);
        versionLabel.setText("v" VERSION, juce::dontSendNotification);
        auto interBoldFont = juce::Typeface::createSystemTypefaceFor(
            BinaryData::Inter_28ptBold_ttf,
            BinaryData::Inter_28ptBold_ttfSize);
        versionLabel.setFont(juce::FontOptions(interBoldFont).withHeight(11.0f));
        versionLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.5f));
        versionLabel.setJustificationType(juce::Justification::centredRight);

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

        // Position settings button in top right corner
        int settingsButtonX = getWidth() - buttonSize - settingsButtonOffsetRight;
        int settingsButtonY = (height - buttonSize) / 2;  // Vertically center
        settingsButton.setBounds(settingsButtonX, settingsButtonY, buttonSize, buttonSize);

        // Position view toggle button left of settings button
        int viewToggleButtonX = settingsButtonX - buttonSize - buttonGap;
        int viewToggleButtonY = (height - buttonSize) / 2;  // Vertically center
        viewToggleButton.setBounds(viewToggleButtonX, viewToggleButtonY, buttonSize, buttonSize);

        // Position version label left of view toggle button
        int versionLabelX = viewToggleButtonX - versionLabelWidth - versionLabelMarginRight;
        int versionLabelY = (height - versionLabelHeight) / 2;  // Vertically center
        versionLabel.setBounds(versionLabelX, versionLabelY, versionLabelWidth, versionLabelHeight);
    }

    // Callback when settings button is clicked
    std::function<void()> onSettingsClicked;

    // Callback when view toggle button is clicked
    // Parameter: true = toggled on (icon 2), false = toggled off (icon 1)
    std::function<void(bool)> onViewToggleClicked;

    // Public method to set settings button toggle state (for ApplicationComponent)
    void setSettingsToggled(bool toggled) {
        settingsButton.setToggled(toggled);
    }

private:
    LogoComponent logo;
    juce::Label versionLabel;
    ToggleIconButton viewToggleButton;
    ToggleIconButton settingsButton;
};

