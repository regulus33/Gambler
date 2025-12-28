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
    // Button indices for clarity
    static constexpr int kRollButton = 0;
    static constexpr int kPageLeftButton = 1;
    static constexpr int kPageRightButton = 2;
    static constexpr int kLengthPlusButton = 3;
    static constexpr int kLengthMinusButton = 4;
    static constexpr int kNudgeLeftButton = 5;
    static constexpr int kNudgeRightButton = 6;
    static constexpr int kShiftButton = 7;

    SequencerControlsComponent() {
        // Create 8 buttons
        for (int i = 0; i < 8; i++) {
            auto* button = new ControlButton();
            buttons.add(button);
            addAndMakeVisible(button);
        }

        // Set button icons and labels (normal mode)
        buttons[kRollButton]->setIcon(Icons::createDiceIcon());
        buttons[kRollButton]->setLabel("Roll");

        buttons[kPageLeftButton]->setIcon(Icons::createIndexLeftIcon());
        buttons[kPageLeftButton]->setLabel("Page");

        buttons[kPageRightButton]->setIcon(Icons::createIndexRightIcon());
        buttons[kPageRightButton]->setLabel("Page");

        buttons[kLengthPlusButton]->setIcon(Icons::createLengthPlusIcon());
        buttons[kLengthPlusButton]->setLabel("Length");

        buttons[kLengthMinusButton]->setIcon(Icons::createLengthMinusIcon());
        buttons[kLengthMinusButton]->setLabel("Length");

        buttons[kNudgeLeftButton]->setIcon(Icons::createNudgeLeftIcon());
        buttons[kNudgeLeftButton]->setLabel("Nudge");

        buttons[kNudgeRightButton]->setIcon(Icons::createNudgeRightIcon());
        buttons[kNudgeRightButton]->setLabel("Nudge");

        buttons[kShiftButton]->setIcon(Icons::createShiftIcon());
        buttons[kShiftButton]->setLabel("Shift");

        // Set toggle state for Shift only (Roll is momentary)
        buttons[kShiftButton]->setToggleable(true);
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

    // Check if Shift mode is active
    bool isShiftActive() const {
        return isButtonToggled(kShiftButton);
    }

    // Update button appearances based on Shift state
    // Called when Shift is toggled to change icons/labels of affected buttons
    void updateShiftedButtonAppearances() {
        bool shifted = isShiftActive();

        if (shifted) {
            // Shifted mode: change affected buttons
            // Page Left -> Duplicate
            buttons[kPageLeftButton]->setIcon(Icons::createLengthPlusIcon());  // Placeholder
            buttons[kPageLeftButton]->setLabel("Duplicate");

            // Page Right -> Clear (clears current sequence window)
            buttons[kPageRightButton]->setIcon(Icons::createCloseIcon());  // Placeholder
            buttons[kPageRightButton]->setLabel("Clear");

            // Length+ -> Clear Step (clears current step to rest)
            buttons[kLengthPlusButton]->setIcon(Icons::createCloseIcon());  // Placeholder
            buttons[kLengthPlusButton]->setLabel("Clear Step");

            // Length- -> Step (randomize current step - will use dice icon)
            buttons[kLengthMinusButton]->setIcon(Icons::createDiceIcon());
            buttons[kLengthMinusButton]->setLabel("Step");

            // Nudge Left -> All (randomize everything everywhere - dice icon)
            buttons[kNudgeLeftButton]->setIcon(Icons::createDiceIcon());
            buttons[kNudgeLeftButton]->setLabel("All");

            // Nudge Right -> Shuffle (randomly move around all steps)
            buttons[kNudgeRightButton]->setIcon(Icons::createDiceIcon());  // Placeholder
            buttons[kNudgeRightButton]->setLabel("Shuffle");
        } else {
            // Normal mode: restore original icons/labels
            buttons[kPageLeftButton]->setIcon(Icons::createIndexLeftIcon());
            buttons[kPageLeftButton]->setLabel("Page");

            buttons[kPageRightButton]->setIcon(Icons::createIndexRightIcon());
            buttons[kPageRightButton]->setLabel("Page");

            buttons[kLengthPlusButton]->setIcon(Icons::createLengthPlusIcon());
            buttons[kLengthPlusButton]->setLabel("Length");

            buttons[kLengthMinusButton]->setIcon(Icons::createLengthMinusIcon());
            buttons[kLengthMinusButton]->setLabel("Length");

            buttons[kNudgeLeftButton]->setIcon(Icons::createNudgeLeftIcon());
            buttons[kNudgeLeftButton]->setLabel("Nudge");

            buttons[kNudgeRightButton]->setIcon(Icons::createNudgeRightIcon());
            buttons[kNudgeRightButton]->setLabel("Nudge");
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
                // Find our index
                int index = parent->buttons.indexOf(this);

                // If Shift button was toggled, update button appearances
                if (index == kShiftButton) {
                    parent->updateShiftedButtonAppearances();
                }

                if (parent->onButtonClicked) {
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

