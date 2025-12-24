#pragma once

#include "Icons.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace SequencerControlsConstants {
    // Layout
    constexpr float buttonSize = 92.0f;
    constexpr float gap = 18.0f;
    // Button appearance
    constexpr float cornerRadius = 4.4f;
    constexpr float borderWidth = 2.2f;
    constexpr juce::uint32 bgColorActive = 0xffdb7c00;
    constexpr juce::uint32 bgColorInactive = 0xff303030;
    // Icon
    constexpr float maxIconSize = 26.0f;
    constexpr float iconVerticalOffset = 13.0f;
    // Text
    constexpr float textAreaHeight = 33.0f;
    constexpr float fontSize = 13.0f;
    constexpr float textHeight = 13.0f;
    constexpr float textYOffset = 13.0f;
}

/**
 * SequencerControlsComponent
 *
 * Displays 8 control buttons below the sequencer steps:
 * - Roll (toggle random mode)
 * - Shift (modifier key)
 * - Page <- (page view window left by 8 steps)
 * - Page -> (page view window right by 8 steps)
 * - Length + (increase sequence length)
 * - Length - (decrease sequence length)
 * - Nudge <- (shift notes left)
 * - Nudge -> (shift notes right)
 *
 * Each button is 92×92 pixels with 18px spacing, matching the SequencerStepsComponent above.
 */
class SequencerControlsComponent : public juce::Component {
public:
    SequencerControlsComponent() {
        // Create 8 buttons
        for (int i = 0; i < 8; i++) {
            auto* button = new ControlButton();
            buttons.add(button);
            addAndMakeVisible(button);
        }
        
        // Set button icons and labels
        buttons[0]->setIcon(Icons::createDiceIcon());
        buttons[0]->setLabel("Roll");

        buttons[1]->setIcon(Icons::createIndexLeftIcon());
        buttons[1]->setLabel("Page");

        buttons[2]->setIcon(Icons::createIndexRightIcon());
        buttons[2]->setLabel("Page");

        buttons[3]->setIcon(Icons::createLengthPlusIcon());
        buttons[3]->setLabel("Length");

        buttons[4]->setIcon(Icons::createLengthMinusIcon());
        buttons[4]->setLabel("Length");

        buttons[5]->setIcon(Icons::createNudgeLeftIcon());
        buttons[5]->setLabel("Nudge");

        buttons[6]->setIcon(Icons::createNudgeRightIcon());
        buttons[6]->setLabel("Nudge");

        buttons[7]->setIcon(Icons::createShiftIcon());
        buttons[7]->setLabel("Shift");

        // Set toggle state for Shift only (Roll is momentary)
        buttons[7]->setToggleable(true);  // Shift
    }
    
    ~SequencerControlsComponent() override {
        buttons.clear(true);
    }
    
    void paint (juce::Graphics& g) override {
        // Background is transparent - buttons draw themselves
    }
    
    void resized() override {
        using namespace SequencerControlsConstants;

        for (int i = 0; i < buttons.size(); i++) {
            float x = i * (buttonSize + gap);
            buttons[i]->setBounds(static_cast<int>(x), 0,
                                 static_cast<int>(buttonSize),
                                 static_cast<int>(buttonSize));
        }
    }
    
    // Callbacks for button clicks
    std::function<void(int buttonIndex)> onButtonClicked;
    std::function<void(int buttonIndex)> onButtonReleased;
    
    // Get button toggle state (for Roll and Shift)
    bool isButtonToggled(int index) const {
        if (index >= 0 && index < buttons.size()) {
            return buttons[index]->isToggled();
        }
        return false;
    }
    
    // Set button toggle state (for Roll and Shift)
    void setButtonToggled(int index, bool toggled) {
        if (index >= 0 && index < buttons.size()) {
            buttons[index]->setToggled(toggled);
        }
    }
    
private:
    /**
     * Individual control button
     * Matches the design: rounded rectangle with white border, grey background
     */
    class ControlButton : public juce::Component {
    public:
        ControlButton() = default;
        
        void paint (juce::Graphics& g) override {
            using namespace SequencerControlsConstants;
            auto bounds = getLocalBounds().toFloat();

            // Determine colors based on toggle state OR pressed state
            bool isActive = isToggled_ || isPressed_;
            juce::Colour bgColor = isActive ? juce::Colour(bgColorActive) : juce::Colour(bgColorInactive);
            juce::Colour borderColor = juce::Colours::white;
            juce::Colour textColor = juce::Colours::white;

            // Fill background
            g.setColour(bgColor);
            g.fillRoundedRectangle(bounds, cornerRadius);

            // Draw white border inside the bounds
            g.setColour(borderColor);
            float inset = borderWidth * 0.5f;
            juce::Rectangle<float> borderBounds = bounds.reduced(inset);
            float borderRadiusInner = juce::jmax(0.0f, cornerRadius - inset);
            g.drawRoundedRectangle(borderBounds, borderRadiusInner, borderWidth);

            // Draw icon and/or label text
            if (iconPath_.has_value()) {
                // Draw the icon path
                g.setColour(textColor);

                // Get the icon's bounding box
                auto iconBounds = iconPath_->getBounds();

                // Calculate scale to limit icon to maxIconSize on longest side
                float longestSide = juce::jmax(iconBounds.getWidth(), iconBounds.getHeight());
                float scale = maxIconSize / longestSide;

                // Calculate the space available for the icon (above the text)
                float iconAreaHeight = bounds.getHeight() - textAreaHeight;

                // Center the icon in the available space, then apply vertical offset
                float scaledWidth = iconBounds.getWidth() * scale;
                float scaledHeight = iconBounds.getHeight() * scale;
                float offsetX = (bounds.getWidth() - scaledWidth) * 0.5f;
                float offsetY = (iconAreaHeight - scaledHeight) * 0.5f + iconVerticalOffset;

                // Apply transform and draw
                juce::AffineTransform transform = juce::AffineTransform::scale(scale)
                    .translated(offsetX - iconBounds.getX() * scale,
                               offsetY - iconBounds.getY() * scale);

                g.fillPath(*iconPath_, transform);
            }

            // Draw label text if present (below icon or centered if no icon)
            if (label_.isNotEmpty()) {
                g.setColour(textColor);
                juce::Font font{juce::FontOptions(fontSize)};
                font.setBold(true);
                g.setFont(font);

                // Position text from bottom
                float textY = bounds.getHeight() - textYOffset - textHeight;
                juce::Rectangle<float> textBounds(0, textY, bounds.getWidth(), textHeight);

                g.drawText(label_, textBounds.toNearestInt(), juce::Justification::centredBottom);
            }
        }
        
        void mouseDown (const juce::MouseEvent& /*event*/) override {
            if (isToggleable_) {
                setToggled(!isToggled_);
            } else {
                // For momentary buttons, show pressed state
                isPressed_ = true;
                repaint();
            }

            // Notify parent
            if (auto* parent = dynamic_cast<SequencerControlsComponent*>(getParentComponent())) {
                if (parent->onButtonClicked) {
                    // Find our index
                    int index = parent->buttons.indexOf(this);
                    parent->onButtonClicked(index);
                }
            }
        }

        void mouseUp (const juce::MouseEvent& /*event*/) override {
            // Clear pressed state for momentary buttons
            if (!isToggleable_ && isPressed_) {
                isPressed_ = false;
                repaint();

                // Notify parent of button release
                if (auto* parent = dynamic_cast<SequencerControlsComponent*>(getParentComponent())) {
                    if (parent->onButtonReleased) {
                        int index = parent->buttons.indexOf(this);
                        parent->onButtonReleased(index);
                    }
                }
            }
        }
        
        void setLabel(const juce::String& label) {
            label_ = label;
            repaint();
        }

        void setIcon(const juce::Path& icon) {
            iconPath_ = icon;
            repaint();
        }

        void setToggleable(bool toggleable) {
            isToggleable_ = toggleable;
        }

        bool isToggled() const {
            return isToggled_;
        }

        void setToggled(bool toggled) {
            if (isToggled_ != toggled) {
                isToggled_ = toggled;
                repaint();
            }
        }

    private:
        juce::String label_;
        std::optional<juce::Path> iconPath_;
        bool isToggleable_ = false;
        bool isToggled_ = false;
        bool isPressed_ = false;  // Track pressed state for momentary buttons
    };
    
    juce::OwnedArray<ControlButton> buttons;
};

