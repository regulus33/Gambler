#pragma once

#include "BinaryData.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace SettingsWindowConstants {
    constexpr int windowWidth = 400;
    constexpr int windowHeight = 300;
    constexpr int padding = 20;
    constexpr juce::uint32 bgColourHex = 0xFF2D2D2D;
    constexpr float titleFontSize = 18.0f;
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
        
        setSize(SettingsWindowConstants::windowWidth, SettingsWindowConstants::windowHeight);
    }

    void paint (juce::Graphics& g) override {
        using namespace SettingsWindowConstants;
        
        // Fill background
        g.fillAll(juce::Colour(bgColourHex));
        
        // Draw placeholder text (will be replaced with actual controls)
        g.setColour(juce::Colours::white);
        juce::Font font(juce::FontOptions(interTypeface).withHeight(14.0f));
        g.setFont(font);
        g.drawText("Settings controls will go here...", 
                   getLocalBounds().reduced(padding), 
                   juce::Justification::centred);
    }

    void resized() override {
        // Layout for future controls will go here
    }

private:
    juce::Typeface::Ptr interTypeface;
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
        // Notify that window is closing
        if (onClose) {
            onClose();
        }
        setVisible(false);
    }

    // Callback when window is closed
    std::function<void()> onClose;
};

