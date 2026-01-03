#pragma once

#include "Icons.h"
#include "StateListenerComponent.h"
#include "buttons/RollButton.h"
#include "buttons/PageLeftButton.h"
#include "buttons/PageRightButton.h"
#include "buttons/LengthPlusButton.h"
#include "buttons/LengthMinusButton.h"
#include "buttons/NudgeLeftButton.h"
#include "buttons/NudgeRightButton.h"
#include "buttons/ShiftButton.h"
#include "buttons/DuplicateButton.h"
#include "buttons/ClearWindowButton.h"
#include "buttons/ClearStepButton.h"
#include "buttons/RandomizeStepButton.h"
#include "buttons/RandomizeAllButton.h"
#include "buttons/ShuffleButton.h"
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
        : StateListenerComponent(processor)
        // Primary row buttons
        , rollButton(processor)
        , pageLeftButton(processor)
        , pageRightButton(processor)
        , lengthPlusButton(processor)
        , lengthMinusButton(processor)
        , nudgeLeftButton(processor)
        , nudgeRightButton(processor)
        , shiftButton(processor)
        // Secondary row buttons
        , duplicateButton(processor)
        , clearWindowButton(processor)
        , clearStepButton(processor)
        , randomizeStepButton(processor)
        , randomizeAllButton(processor)
        , shuffleButton(processor)
    {
        // Subscribe to APVTS parameters for secondary row visibility
        listenToParameter(Params::Keys::ID::SEQ_IS_SHIFT);

        // ========== PRIMARY ROW - Add new observer-pattern buttons ==========
        addAndMakeVisible(rollButton);
        addAndMakeVisible(pageLeftButton);
        addAndMakeVisible(pageRightButton);
        addAndMakeVisible(lengthPlusButton);
        addAndMakeVisible(lengthMinusButton);
        addAndMakeVisible(nudgeLeftButton);
        addAndMakeVisible(nudgeRightButton);
        addAndMakeVisible(shiftButton);

        // ========== SECONDARY ROW - Add new observer-pattern buttons ==========
        addAndMakeVisible(duplicateButton);
        addAndMakeVisible(clearWindowButton);
        addAndMakeVisible(clearStepButton);
        addAndMakeVisible(randomizeStepButton);
        addAndMakeVisible(randomizeAllButton);
        addAndMakeVisible(shuffleButton);

        // Initially hide secondary row (Shift not active)
        updateSecondaryRowVisibility();
    }

    ~SequencerControlsComponent() override = default;

    void paint (juce::Graphics& g) override {
        // Background is transparent - buttons draw themselves
        juce::ignoreUnused(g);
    }

    void resized() override {
        using namespace SequencerControlsConstants;
        int size = static_cast<int>(buttonSize);
        int spacing = static_cast<int>(buttonSize + gap);

        // ========== PRIMARY ROW ==========
        rollButton.setBounds(kRollButton * spacing, 0, size, size);
        pageLeftButton.setBounds(kPageLeftButton * spacing, 0, size, size);
        pageRightButton.setBounds(kPageRightButton * spacing, 0, size, size);
        lengthPlusButton.setBounds(kLengthPlusButton * spacing, 0, size, size);
        lengthMinusButton.setBounds(kLengthMinusButton * spacing, 0, size, size);
        nudgeLeftButton.setBounds(kNudgeLeftButton * spacing, 0, size, size);
        nudgeRightButton.setBounds(kNudgeRightButton * spacing, 0, size, size);
        shiftButton.setBounds(kShiftButton * spacing, 0, size, size);

        // ========== SECONDARY ROW ==========
        // Secondary buttons align with primary buttons at indices 1-6
        int secondaryY = static_cast<int>(buttonSize + gap);
        duplicateButton.setBounds(1 * spacing, secondaryY, size, size);
        clearWindowButton.setBounds(2 * spacing, secondaryY, size, size);
        clearStepButton.setBounds(3 * spacing, secondaryY, size, size);
        randomizeStepButton.setBounds(4 * spacing, secondaryY, size, size);
        randomizeAllButton.setBounds(5 * spacing, secondaryY, size, size);
        shuffleButton.setBounds(6 * spacing, secondaryY, size, size);
    }

    // Check if Shift mode is active - reads from APVTS for consistency
    bool isShiftActive() const {
        return getState().getIsShift();
    }

    // Set the MIDI learn registry (called from parent)
    void setRegistry(MidiLearnRegistry* reg) {
        registry = reg;
        // Pass to all observer-pattern buttons
        rollButton.setRegistry(reg);
        pageLeftButton.setRegistry(reg);
        pageRightButton.setRegistry(reg);
        lengthPlusButton.setRegistry(reg);
        lengthMinusButton.setRegistry(reg);
        nudgeLeftButton.setRegistry(reg);
        nudgeRightButton.setRegistry(reg);
        shiftButton.setRegistry(reg);
        duplicateButton.setRegistry(reg);
        clearWindowButton.setRegistry(reg);
        clearStepButton.setRegistry(reg);
        randomizeStepButton.setRegistry(reg);
        randomizeAllButton.setRegistry(reg);
        shuffleButton.setRegistry(reg);
    }

    // Update secondary row visibility based on Shift state
    void updateSecondaryRowVisibility() {
        bool showSecondary = isShiftActive();
        duplicateButton.setVisible(showSecondary);
        clearWindowButton.setVisible(showSecondary);
        clearStepButton.setVisible(showSecondary);
        randomizeStepButton.setVisible(showSecondary);
        randomizeAllButton.setVisible(showSecondary);
        shuffleButton.setVisible(showSecondary);
    }

protected:
    // ========================================
    // StateListenerComponent implementation
    // ========================================

    /**
     * Called on message thread when SEQ_IS_SHIFT changes.
     * Updates secondary row visibility.
     * Note: Individual button visual states are handled by each button's own
     * StateListenerComponent - they observe their own params.
     */
    void onStateChanged() override {
        // Update secondary row visibility based on shift state
        updateSecondaryRowVisibility();
    }

private:
    MidiLearnRegistry* registry = nullptr;

    // ========== PRIMARY ROW BUTTONS ==========
    RollButton rollButton;
    PageLeftButton pageLeftButton;
    PageRightButton pageRightButton;
    LengthPlusButton lengthPlusButton;
    LengthMinusButton lengthMinusButton;
    NudgeLeftButton nudgeLeftButton;
    NudgeRightButton nudgeRightButton;
    ShiftButton shiftButton;

    // ========== SECONDARY ROW BUTTONS ==========
    DuplicateButton duplicateButton;
    ClearWindowButton clearWindowButton;
    ClearStepButton clearStepButton;
    RandomizeStepButton randomizeStepButton;
    RandomizeAllButton randomizeAllButton;
    ShuffleButton shuffleButton;
};

