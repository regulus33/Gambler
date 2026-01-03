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
 * Displays a single row of 8 control buttons below the sequencer steps.
 * When Shift is toggled, buttons at positions 1-6 are replaced with alternate functions.
 *
 * NORMAL MODE (Shift OFF):
 * Position 0: Roll (momentary: enable random mode while held)
 * Position 1: Page <- (page view window left by 8 steps)
 * Position 2: Page -> (page view window right by 8 steps)
 * Position 3: Length + (increase sequence length)
 * Position 4: Length - (decrease sequence length)
 * Position 5: Nudge <- (shift all notes left)
 * Position 6: Nudge -> (shift all notes right)
 * Position 7: Shift (toggle to swap buttons)
 *
 * SHIFT MODE (Shift ON):
 * Position 0: Roll (unchanged)
 * Position 1: Duplicate (double sequence length, copy content)
 * Position 2: Clear Window (clear current 8-step window)
 * Position 3: Clear Step (clear current step to rest)
 * Position 4: Randomize All (randomize all steps)
 * Position 5: Randomize Step (randomize current step)
 * Position 6: Shuffle (shuffle step order)
 * Position 7: Shift (unchanged)
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
    // Button position indices
    static constexpr int kPosition0 = 0;  // Roll (always)
    static constexpr int kPosition1 = 1;  // Page Left / Duplicate
    static constexpr int kPosition2 = 2;  // Page Right / Clear Window
    static constexpr int kPosition3 = 3;  // Length+ / Clear Step
    static constexpr int kPosition4 = 4;  // Length- / Randomize All
    static constexpr int kPosition5 = 5;  // Nudge Left / Randomize Step
    static constexpr int kPosition6 = 6;  // Nudge Right / Shuffle
    static constexpr int kPosition7 = 7;  // Shift (always)

    explicit SequencerControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor)
        // Always-visible buttons
        , rollButton(processor)
        , shiftButton(processor)
        // Normal mode buttons (positions 1-6)
        , pageLeftButton(processor)
        , pageRightButton(processor)
        , lengthPlusButton(processor)
        , lengthMinusButton(processor)
        , nudgeLeftButton(processor)
        , nudgeRightButton(processor)
        // Shift mode buttons (positions 1-6)
        , duplicateButton(processor)
        , clearWindowButton(processor)
        , clearStepButton(processor)
        , randomizeAllButton(processor)
        , randomizeStepButton(processor)
        , shuffleButton(processor)
    {
        // Subscribe to shift state changes
        listenToParameter(Params::Keys::ID::SEQ_IS_SHIFT);

        // ========== ALWAYS VISIBLE ==========
        addAndMakeVisible(rollButton);
        addAndMakeVisible(shiftButton);

        // ========== NORMAL MODE BUTTONS (positions 1-6) ==========
        addAndMakeVisible(pageLeftButton);
        addAndMakeVisible(pageRightButton);
        addAndMakeVisible(lengthPlusButton);
        addAndMakeVisible(lengthMinusButton);
        addAndMakeVisible(nudgeLeftButton);
        addAndMakeVisible(nudgeRightButton);

        // ========== SHIFT MODE BUTTONS (positions 1-6) ==========
        addChildComponent(duplicateButton);      // Hidden initially
        addChildComponent(clearWindowButton);
        addChildComponent(clearStepButton);
        addChildComponent(randomizeAllButton);
        addChildComponent(randomizeStepButton);
        addChildComponent(shuffleButton);

        // Set initial visibility based on shift state
        updateButtonVisibility();
    }

    ~SequencerControlsComponent() override = default;

    void paint(juce::Graphics& g) override {
        juce::ignoreUnused(g);
    }

    void resized() override {
        using namespace SequencerControlsConstants;
        int size = static_cast<int>(buttonSize);
        int spacing = static_cast<int>(buttonSize + gap);

        // Position 0: Roll (always visible)
        rollButton.setBounds(kPosition0 * spacing, 0, size, size);

        // Position 7: Shift (always visible)
        shiftButton.setBounds(kPosition7 * spacing, 0, size, size);

        // Positions 1-6: Normal mode buttons
        pageLeftButton.setBounds(kPosition1 * spacing, 0, size, size);
        pageRightButton.setBounds(kPosition2 * spacing, 0, size, size);
        lengthPlusButton.setBounds(kPosition3 * spacing, 0, size, size);
        lengthMinusButton.setBounds(kPosition4 * spacing, 0, size, size);
        nudgeLeftButton.setBounds(kPosition5 * spacing, 0, size, size);
        nudgeRightButton.setBounds(kPosition6 * spacing, 0, size, size);

        // Positions 1-6: Shift mode buttons (same positions, different buttons)
        duplicateButton.setBounds(kPosition1 * spacing, 0, size, size);
        clearWindowButton.setBounds(kPosition2 * spacing, 0, size, size);
        clearStepButton.setBounds(kPosition3 * spacing, 0, size, size);
        randomizeAllButton.setBounds(kPosition4 * spacing, 0, size, size);
        randomizeStepButton.setBounds(kPosition5 * spacing, 0, size, size);
        shuffleButton.setBounds(kPosition6 * spacing, 0, size, size);
    }

    bool isShiftActive() const {
        return getState().getIsShift();
    }

    void setRegistry(MidiLearnRegistry* reg) {
        registry = reg;
        // Pass to all buttons
        rollButton.setRegistry(reg);
        shiftButton.setRegistry(reg);
        pageLeftButton.setRegistry(reg);
        pageRightButton.setRegistry(reg);
        lengthPlusButton.setRegistry(reg);
        lengthMinusButton.setRegistry(reg);
        nudgeLeftButton.setRegistry(reg);
        nudgeRightButton.setRegistry(reg);
        duplicateButton.setRegistry(reg);
        clearWindowButton.setRegistry(reg);
        clearStepButton.setRegistry(reg);
        randomizeAllButton.setRegistry(reg);
        randomizeStepButton.setRegistry(reg);
        shuffleButton.setRegistry(reg);
    }

    void updateButtonVisibility() {
        bool shiftOn = isShiftActive();

        // Normal mode buttons: visible when shift is OFF
        pageLeftButton.setVisible(!shiftOn);
        pageRightButton.setVisible(!shiftOn);
        lengthPlusButton.setVisible(!shiftOn);
        lengthMinusButton.setVisible(!shiftOn);
        nudgeLeftButton.setVisible(!shiftOn);
        nudgeRightButton.setVisible(!shiftOn);

        // Shift mode buttons: visible when shift is ON
        duplicateButton.setVisible(shiftOn);
        clearWindowButton.setVisible(shiftOn);
        clearStepButton.setVisible(shiftOn);
        randomizeAllButton.setVisible(shiftOn);
        randomizeStepButton.setVisible(shiftOn);
        shuffleButton.setVisible(shiftOn);
    }

protected:
    void onStateChanged() override {
        updateButtonVisibility();
    }

private:
    MidiLearnRegistry* registry = nullptr;

    // ========== ALWAYS VISIBLE ==========
    RollButton rollButton;
    ShiftButton shiftButton;

    // ========== NORMAL MODE BUTTONS (positions 1-6) ==========
    PageLeftButton pageLeftButton;
    PageRightButton pageRightButton;
    LengthPlusButton lengthPlusButton;
    LengthMinusButton lengthMinusButton;
    NudgeLeftButton nudgeLeftButton;
    NudgeRightButton nudgeRightButton;

    // ========== SHIFT MODE BUTTONS (positions 1-6) ==========
    DuplicateButton duplicateButton;
    ClearWindowButton clearWindowButton;
    ClearStepButton clearStepButton;
    RandomizeAllButton randomizeAllButton;
    RandomizeStepButton randomizeStepButton;
    ShuffleButton shuffleButton;
};

