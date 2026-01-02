#pragma once

#include "Icons.h"
#include "StateListenerComponent.h"
#include "../backend/MidiLearnRegistry.h"
#include "../data/Params.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace SequencerControlsConstants {
    // Layout
    constexpr float buttonSize = 92.0f;
    constexpr float gap = 18.0f;
    // Button appearance
    constexpr float cornerRadius = 4.4f;
    constexpr float borderWidth = 2.2f;
    constexpr juce::uint32 bgColorActive = 0xffffffff;     // White when active
    constexpr juce::uint32 bgColorInactive = 0xff303030;  // Dark grey when inactive
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
 * Displays two rows of control buttons below the sequencer steps:
 *
 * PRIMARY ROW (always visible, 8 buttons):
 * - Roll (momentary: enable random mode while held)
 * - Page <- (page view window left by 8 steps)
 * - Page -> (page view window right by 8 steps)
 * - Length + (increase sequence length)
 * - Length - (decrease sequence length)
 * - Nudge <- (shift all notes left)
 * - Nudge -> (shift all notes right)
 * - Shift (toggle to show/hide secondary row)
 *
 * SECONDARY ROW (visible when Shift is active, 6 buttons):
 * - Duplicate (double sequence length, copy content)
 * - Clear (clear current 8-step window)
 * - Clear Step (clear current step to rest)
 * - Randomize Step (randomize current step)
 * - Randomize All (randomize all steps)
 * - Shuffle (shuffle step order)
 *
 * Each button is 92×92 pixels with 18px spacing, matching the SequencerStepsComponent above.
 *
 * APVTS-Driven State:
 * - Roll button visual state is driven by SEQ_IS_RANDOM parameter
 * - Shift button visual state is driven by SEQ_IS_SHIFT parameter
 * - This ensures UI updates automatically when parameters change from any source
 *   (UI click, MIDI controller, DAW automation)
 */
class SequencerControlsComponent : public StateListenerComponent {
public:
    // Primary row button indices (0-7)
    static constexpr int kRollButton = 0;
    static constexpr int kPageLeftButton = 1;
    static constexpr int kPageRightButton = 2;
    static constexpr int kLengthPlusButton = 3;
    static constexpr int kLengthMinusButton = 4;
    static constexpr int kNudgeLeftButton = 5;
    static constexpr int kNudgeRightButton = 6;
    static constexpr int kShiftButton = 7;

    // Secondary row button indices (8-13)
    static constexpr int kDuplicateButton = 8;
    static constexpr int kClearWindowButton = 9;
    static constexpr int kClearStepButton = 10;
    static constexpr int kRandomizeStepButton = 11;
    static constexpr int kRandomizeAllButton = 12;
    static constexpr int kShuffleButton = 13;

    static constexpr int kPrimaryRowCount = 8;
    static constexpr int kSecondaryRowCount = 6;
    static constexpr int kTotalButtons = kPrimaryRowCount + kSecondaryRowCount;

    explicit SequencerControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor) {

        // Subscribe to APVTS parameters that drive button visual state
        listenToParameter(Params::Keys::ID::SEQ_IS_RANDOM);
        listenToParameter(Params::Keys::ID::SEQ_IS_SHIFT);

        // Create all buttons (primary + secondary rows)
        for (int i = 0; i < kTotalButtons; i++) {
            auto* button = new ControlButton();
            buttons.add(button);
            addAndMakeVisible(button);
        }

        // ========== PRIMARY ROW SETUP ==========
        buttons[kRollButton]->setIcon(Icons::createDiceIcon());
        buttons[kRollButton]->setLabel("Roll");
        buttons[kRollButton]->setMidiControlId(MidiControlId::BUTTON_ROLL);

        buttons[kPageLeftButton]->setIcon(Icons::createIndexLeftIcon());
        buttons[kPageLeftButton]->setLabel("Page");
        buttons[kPageLeftButton]->setMidiControlId(MidiControlId::BUTTON_PAGE_LEFT);

        buttons[kPageRightButton]->setIcon(Icons::createIndexRightIcon());
        buttons[kPageRightButton]->setLabel("Page");
        buttons[kPageRightButton]->setMidiControlId(MidiControlId::BUTTON_PAGE_RIGHT);

        buttons[kLengthPlusButton]->setIcon(Icons::createLengthPlusIcon());
        buttons[kLengthPlusButton]->setLabel("Length");
        buttons[kLengthPlusButton]->setMidiControlId(MidiControlId::BUTTON_LENGTH_PLUS);

        buttons[kLengthMinusButton]->setIcon(Icons::createLengthMinusIcon());
        buttons[kLengthMinusButton]->setLabel("Length");
        buttons[kLengthMinusButton]->setMidiControlId(MidiControlId::BUTTON_LENGTH_MINUS);

        buttons[kNudgeLeftButton]->setIcon(Icons::createNudgeLeftIcon());
        buttons[kNudgeLeftButton]->setLabel("Nudge");
        buttons[kNudgeLeftButton]->setMidiControlId(MidiControlId::BUTTON_NUDGE_LEFT);

        buttons[kNudgeRightButton]->setIcon(Icons::createNudgeRightIcon());
        buttons[kNudgeRightButton]->setLabel("Nudge");
        buttons[kNudgeRightButton]->setMidiControlId(MidiControlId::BUTTON_NUDGE_RIGHT);

        buttons[kShiftButton]->setIcon(Icons::createShiftIcon());
        buttons[kShiftButton]->setLabel("Shift");
        buttons[kShiftButton]->setMidiControlId(MidiControlId::BUTTON_SHIFT);
        buttons[kShiftButton]->setToggleable(true);

        // ========== SECONDARY ROW SETUP ==========
        buttons[kDuplicateButton]->setIcon(Icons::createDupIcon());
        buttons[kDuplicateButton]->setLabel("Duplicate");
        buttons[kDuplicateButton]->setMidiControlId(MidiControlId::BUTTON_DUPLICATE);

        buttons[kClearWindowButton]->setIcon(Icons::createTrashIcon());
        buttons[kClearWindowButton]->setLabel("Clear");
        buttons[kClearWindowButton]->setMidiControlId(MidiControlId::BUTTON_CLEAR_WINDOW);

        buttons[kClearStepButton]->setIcon(Icons::createEraserIcon());
        buttons[kClearStepButton]->setLabel("Clear Step");
        buttons[kClearStepButton]->setMidiControlId(MidiControlId::BUTTON_CLEAR_STEP);

        buttons[kRandomizeStepButton]->setIcon(Icons::createDiceOneIcon());
        buttons[kRandomizeStepButton]->setLabel("Step");
        buttons[kRandomizeStepButton]->setMidiControlId(MidiControlId::BUTTON_RANDOMIZE_STEP);

        buttons[kRandomizeAllButton]->setIcon(Icons::createRandomAllIcon());
        buttons[kRandomizeAllButton]->setLabel("All");
        buttons[kRandomizeAllButton]->setMidiControlId(MidiControlId::BUTTON_RANDOMIZE_ALL);

        buttons[kShuffleButton]->setIcon(Icons::createShuffleIcon());
        buttons[kShuffleButton]->setLabel("Shuffle");
        buttons[kShuffleButton]->setMidiControlId(MidiControlId::BUTTON_SHUFFLE);

        // Initially hide secondary row (Shift not active)
        updateSecondaryRowVisibility();
    }

    ~SequencerControlsComponent() override {
        buttons.clear(true);
    }

    void paint (juce::Graphics& g) override {
        // Background is transparent - buttons draw themselves
    }

    void resized() override {
        using namespace SequencerControlsConstants;

        // Layout primary row (always visible)
        for (int i = 0; i < kPrimaryRowCount; i++) {
            float x = i * (buttonSize + gap);
            buttons[i]->setBounds(static_cast<int>(x), 0,
                                 static_cast<int>(buttonSize),
                                 static_cast<int>(buttonSize));
        }

        // Layout secondary row (below primary, positions 1-6 to align with Page Left through Nudge Right)
        // Secondary buttons align with primary buttons at indices 1-6
        for (int i = 0; i < kSecondaryRowCount; i++) {
            int primaryAlignIndex = i + 1;  // Align with Page Left (1) through Nudge Right (6)
            float x = primaryAlignIndex * (buttonSize + gap);
            float y = buttonSize + gap;  // Below primary row
            buttons[kPrimaryRowCount + i]->setBounds(static_cast<int>(x), static_cast<int>(y),
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

    // Set button pressed state (for momentary buttons like Roll)
    void setButtonPressed(int index, bool pressed) {
        if (index >= 0 && index < buttons.size()) {
            buttons[index]->setPressed(pressed);
        }
    }

    // Check if Shift mode is active - reads from APVTS for consistency
    bool isShiftActive() const {
        return getState().getIsShift();
    }

    // Set the MIDI learn registry (called from parent)
    void setRegistry(MidiLearnRegistry* reg) {
        registry = reg;
        // Pass to all buttons
        for (auto* button : buttons) {
            button->setRegistry(reg);
        }
    }

    // Update secondary row visibility based on Shift state
    void updateSecondaryRowVisibility() {
        bool showSecondary = isShiftActive();
        for (int i = kPrimaryRowCount; i < kTotalButtons; i++) {
            buttons[i]->setVisible(showSecondary);
        }
    }

protected:
    // ========================================
    // StateListenerComponent implementation
    // ========================================

    /**
     * Called on message thread when SEQ_IS_RANDOM or SEQ_IS_SHIFT changes.
     * Updates button visual states to reflect APVTS state.
     */
    void onStateChanged() override {
        // Read current state from APVTS
        bool isRandom = getState().getIsRandom();
        bool isShift = getState().getIsShift();

        // Update Roll button visual state (momentary button shows pressed when random is active)
        buttons[kRollButton]->setPressed(isRandom);

        // Update Shift button visual state (toggle button)
        buttons[kShiftButton]->setToggled(isShift);

        // Update secondary row visibility based on shift state
        updateSecondaryRowVisibility();
    }

private:
    /**
     * Individual control button
     * Matches the design: rounded rectangle with white border, grey background
     */
    class ControlButton : public juce::Component {
    public:
        ControlButton() = default;

        void setRegistry(MidiLearnRegistry* reg) {
            registry = reg;
            updateRegistration();
        }

        void setMidiControlId(MidiControlId id) {
            controlId = id;
            updateRegistration();
        }
        
        void paint (juce::Graphics& g) override {
            using namespace SequencerControlsConstants;
            auto bounds = getLocalBounds().toFloat();

            // Determine colors based on toggle state OR pressed state
            bool isActive = isToggled_ || isPressed_;
            juce::Colour bgColor = isActive ? juce::Colour(bgColorActive) : juce::Colour(bgColorInactive);
            juce::Colour borderColor = juce::Colours::white;
            juce::Colour textColor = isActive ? juce::Colours::black : juce::Colours::white;
            juce::Colour iconColor = isActive ? juce::Colours::black : juce::Colours::white;

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
                g.setColour(iconColor);

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

            // Draw MIDI learn overlay if in learn mode
            paintMidiLearnOverlay(g);
        }

        void paintMidiLearnOverlay(juce::Graphics& g) {
            if (!registry || !registry->isLearning()) return;
            if (controlId == MidiControlId::COUNT) return;

            auto bounds = getLocalBounds();

            // Determine if this control is currently being learned
            bool isLearningThisControl = (registry->getLearningControl() == controlId);

            // Semi-transparent blue rectangle
            auto overlayColor = juce::Colour(0x4080CCFF);  // Blue with ~25% opacity
            g.setColour(overlayColor);
            g.fillRoundedRectangle(bounds.toFloat(), SequencerControlsConstants::cornerRadius);

            // White border if actively learning
            if (isLearningThisControl) {
                g.setColour(juce::Colours::white);
                g.drawRoundedRectangle(bounds.toFloat().reduced(1.5f), SequencerControlsConstants::cornerRadius, 3.0f);
            }

            // Display MIDI mapping info (if mapped)
            const auto* midiId = registry->getMidiForControl(controlId);
            if (midiId) {
                g.setColour(juce::Colours::white);
                g.setFont(9.0f);
                g.drawText(midiId->toString(),
                          bounds,
                          juce::Justification::centred);
            }
        }

        // This is the outermost captured click event for any button in this component, it then redirects
        // this event to the appropriate callback related to that button.
        // if shift we expose the shift buttons. (or we're supposed to but this is a bug now)
        // if its within the functional index, we call the
        void mouseDown (const juce::MouseEvent& event) override {
            // Check if in MIDI learn mode first
            if (registry && registry->isLearning() && controlId != MidiControlId::COUNT) {
                // Set this control as the learning target
                registry->setLearningControl(controlId);
                repaint();
                return;  // Don't trigger normal button action
            }

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

                // If Shift button was toggled, update secondary row visibility
                if (index == kShiftButton) {
                    parent->updateSecondaryRowVisibility();
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

        void setPressed(bool pressed) {
            if (isPressed_ != pressed) {
                isPressed_ = pressed;
                repaint();
            }
        }

        bool isPressed() const {
            return isPressed_;
        }

        void resized() override {
            updateBoundsInRegistry();
        }

    private:
        void updateRegistration() {
            if (registry && controlId != MidiControlId::COUNT && isShowing()) {
                auto screenBounds = getScreenBounds();
                registry->registerControl(controlId, screenBounds, getMidiControlName(controlId));
            }
        }

        void updateBoundsInRegistry() {
            if (registry && controlId != MidiControlId::COUNT) {
                auto screenBounds = getScreenBounds();
                registry->updateControlBounds(controlId, screenBounds);
            }
        }

        juce::String label_;
        std::optional<juce::Path> iconPath_;
        bool isToggleable_ = false;
        bool isToggled_ = false;
        bool isPressed_ = false;  // Track pressed state for momentary buttons
        MidiLearnRegistry* registry = nullptr;
        MidiControlId controlId = MidiControlId::COUNT;
    };

    juce::OwnedArray<ControlButton> buttons;
    MidiLearnRegistry* registry = nullptr;
};

