#pragma once

#include "BinaryData.h"
#include "LogoComponent.h"
#include "SettingsButtonComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace NavBarConstants {
    constexpr int height = 60;  // Height of the navbar
    constexpr int logoGapFromTop = 10;  // Gap between top and logo
    constexpr int versionLabelWidth = 60;
    constexpr int versionLabelHeight = 20;
    constexpr int versionLabelMarginRight = 10;  // Gap between version and settings button
    constexpr int settingsButtonSize = SettingsButtonConstants::totalSize;  // 36px
    constexpr int settingsButtonOffsetRight = 46;  // Offset from right edge
}

/**
 * NavBarComponent
 *
 * A navigation bar component that displays:
 * - Logo (left side, centered vertically)
 * - Version label (right side, left of settings button)
 * - Settings button (right side)
 *
 * This component is displayed at the top of all screens.
 */
class NavBarComponent : public juce::Component {
public:
    NavBarComponent() {
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

        // Add settings button
        addAndMakeVisible(settingsButton);
        settingsButton.onClick = [this]() {
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
        int settingsButtonX = getWidth() - settingsButtonSize - settingsButtonOffsetRight;
        int settingsButtonY = (height - settingsButtonSize) / 2;  // Vertically center
        settingsButton.setBounds(settingsButtonX, settingsButtonY, settingsButtonSize, settingsButtonSize);

        // Position version label left of settings button
        int versionLabelX = settingsButtonX - versionLabelWidth - versionLabelMarginRight;
        int versionLabelY = (height - versionLabelHeight) / 2;  // Vertically center
        versionLabel.setBounds(versionLabelX, versionLabelY, versionLabelWidth, versionLabelHeight);
    }

    // Callback when settings button is clicked
    std::function<void()> onSettingsClicked;

private:
    LogoComponent logo;
    juce::Label versionLabel;
    SettingsButtonComponent settingsButton;
};

