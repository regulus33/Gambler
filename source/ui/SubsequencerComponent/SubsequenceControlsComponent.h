#pragma once

#include "SingleBarComponent.h"
#include "SubsequenceBarComponent.h"
#include "SubsequenceButtonComponent.h"
#include "../StateListenerComponent.h"
#include "../MusicConstants.h"
#include "../UIConstants.h"
#include "../../backend/MidiLearnRegistry.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * SubsequenceControlsComponent
 *
 * Container for the Subsequence controls UI.
 * - Title "Sub" at the top
 * - SubsequenceBarComponent (8 vertical clickable rectangles)
 * - Two buttons: "-" and "+"
 *
 * Observer Pattern:
 * - Listens to selected step and substep parameters
 * - Automatically updates UI when substep data changes
 *
 * MIDI Learn:
 * - Supports MIDI mapping for SUBSEQ_BAR_0-7, SUBSEQ_MINUS, SUBSEQ_PLUS
 */
class SubsequenceControlsComponent : public StateListenerComponent {
public:
    explicit SubsequenceControlsComponent(PluginProcessor& processor)
        : StateListenerComponent(processor),
          minusButton ("-"),
          plusButton ("+")
    {
        addAndMakeVisible (subsequenceBar);
        addAndMakeVisible (minusButton);
        addAndMakeVisible (plusButton);

        // Set MIDI control IDs for buttons
        minusButton.setMidiControlId(MidiControlId::SUBSEQ_MINUS);
        plusButton.setMidiControlId(MidiControlId::SUBSEQ_PLUS);

        // Setup bar click callback (select for editing)
        subsequenceBar.onBarClicked = [this] (int index) {
            if (onBarClicked)
                onBarClicked (index);
        };

        // Setup bar double-click callback (set as active/current)
        subsequenceBar.onBarDoubleClicked = [this] (int index) {
            if (onBarDoubleClicked)
                onBarDoubleClicked(index);
        };

        // Setup button callbacks
        minusButton.onClick = [this]() {
            if (onMinusClicked)
                onMinusClicked();
        };

        plusButton.onClick = [this]() {
            if (onPlusClicked)
                onPlusClicked();
        };

        // Subscribe to sequence metadata (selected step, current index, length)
        listenToSequenceMetadata();
        listenToStepNotes(0, ParamConstants::MAX_STEPS - 1);
        listenToStepNotesLength(0, ParamConstants::MAX_STEPS - 1);

        // Subscribe to note colour changes
        for (int i = 0; i < 12; ++i)
            listenToParameter(Params::Keys::ID::noteColour(i));
    }

    /**
     * Set the MIDI learn registry for all child components
     */
    void setMidiLearnRegistry(MidiLearnRegistry* registry) {
        subsequenceBar.setRegistry(registry);
        minusButton.setRegistry(registry);
        plusButton.setRegistry(registry);
    }

    void resized() override {
        using namespace SubsequenceControlsConstants;

        int barY = gapBelowTitle;

        // Center the bar horizontally
        int barX = (getWidth() - static_cast<int>(barWidth)) / 2;
        subsequenceBar.setBounds (barX, barY, static_cast<int>(barWidth), barStackHeight);

        // Position buttons below the bar
        int buttonsY = subsequenceBar.getBottom() + gapAboveButtons;

        // Calculate button positions so they align with bar edges
        // Left edge of "-" aligns with left edge of bar
        // Right edge of "+" aligns with right edge of bar
        int buttonWidthInt = static_cast<int>(std::ceil(buttonWidth));
        int totalButtonsWidth = 2 * buttonWidthInt;
        int spacing = static_cast<int>(barWidth) - totalButtonsWidth;

        minusButton.setBounds (barX, buttonsY, buttonWidthInt, buttonHeight);
        plusButton.setBounds (barX + buttonWidthInt + spacing, buttonsY, buttonWidthInt, buttonHeight);
    }

    // Access to the bar for external updates
    SubsequenceBarComponent& getSubsequenceBar() { return subsequenceBar; }
    const SubsequenceBarComponent& getSubsequenceBar() const { return subsequenceBar; }

    // Callbacks
    std::function<void(int)> onBarClicked;
    std::function<void(int)> onBarDoubleClicked;
    std::function<void()> onMinusClicked;
    std::function<void()> onPlusClicked;

protected:
    /**
     * Called on message thread when any subscribed parameter changes.
     * Updates the subsequence bar to reflect the current substep state.
     */
    void onStateChanged() override {
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        int substepLength = state.getStepNotesLength(selectedStep);
        int nextSubstep = state.getNextSubstepIndex(selectedStep);
        int selectedSubstep = state.getSelectedSubstepIndex(selectedStep);

        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            if (i < substepLength) {
                int noteNumber = state.getQuantizedStepNote(selectedStep, i);
                bool isActive = (i == nextSubstep);

                if (noteNumber < 0) {
                    if (isActive)
                        subsequenceBar.setBarColor(i, UIConstants::activeStepColour);
                    else
                        subsequenceBar.setBarColor(i, juce::Colour(SubsequenceControlsConstants::restGreyHex));
                    subsequenceBar.setBarNoteLabel(i, "");
                } else {
                    if (isActive) {
                        subsequenceBar.setBarColor(i, UIConstants::activeStepColour);
                    } else {
                        int pitchClass = noteNumber % 12;
                        subsequenceBar.setBarColor(i, state.getNoteColour(pitchClass));
                    }
                    subsequenceBar.setBarNoteLabel(i, MusicConstants::midiNoteToNoteName(noteNumber));
                }

                subsequenceBar.setBarCurrent(i, isActive);

                // Border priority: active > selected > next
                // Active rests use white fill instead of white border
                if (isActive && noteNumber >= 0) {
                    subsequenceBar.setBarBorderStyle(i, BarBorderStyle::current);
                } else if (isActive && noteNumber < 0) {
                    subsequenceBar.setBarBorderStyle(i, BarBorderStyle::none);
                } else if (i == selectedSubstep) {
                    subsequenceBar.setBarBorderStyle(i, BarBorderStyle::selected);
                } else {
                    subsequenceBar.setBarBorderStyle(i, BarBorderStyle::none);
                }
            } else {
                subsequenceBar.setBarColor(i, juce::Colour(SubsequenceControlsConstants::defaultGreyHex));
                subsequenceBar.setBarNoteLabel(i, "");
                subsequenceBar.setBarCurrent(i, false);
                subsequenceBar.setBarBorderStyle(i, BarBorderStyle::none);
            }
        }
    }

private:
    SubsequenceBarComponent subsequenceBar;
    SubsequenceButtonComponent minusButton;
    SubsequenceButtonComponent plusButton;
};
