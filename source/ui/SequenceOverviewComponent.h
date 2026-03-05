#pragma once

#include "SequenceGrid.h"
#include "StateListenerComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace SequenceOverviewConstants {
    // Container layout
    constexpr int containerWidth = 198;
    // Grid positioning (centered within container)
    constexpr int gridHorizontalPadding = (containerWidth - SequenceGridConstants::gridWidth) / 2;
}

/**
 * SequenceOverviewComponent
 *
 * Top-level container for the Sequence Overview UI.
 * - SequenceGrid (8x8 grid of colored squares)
 *
 * Observer Pattern:
 * - Listens to all step note parameters and sequence metadata
 * - Automatically updates the SequenceGrid when state changes
 */
class SequenceOverviewComponent : public StateListenerComponent {
public:
    explicit SequenceOverviewComponent(PluginProcessor& processor)
        : StateListenerComponent(processor)
    {
        addAndMakeVisible (sequenceGrid);

        // Subscribe to all step notes (for color updates)
        listenToStepNotes(0, 63);
        listenToStepNotesLength(0, 63);

        // Subscribe to sequence metadata (for length, current index, selected step)
        listenToSequenceMetadata();
    }

    void resized() override {
        using namespace SequenceOverviewConstants;

        // Center the grid horizontally
        sequenceGrid.setBounds (
            gridHorizontalPadding,
            0,
            SequenceGridConstants::gridWidth,
            SequenceGridConstants::gridHeight);
    }

    // Access to the grid for external updates
    SequenceGrid& getSequenceGrid() { return sequenceGrid; }
    const SequenceGrid& getSequenceGrid() const { return sequenceGrid; }

protected:
    /**
     * Called on message thread when any subscribed parameter changes.
     * Updates the SequenceGrid from APVTS state.
     */
    void onStateChanged() override {
        // Get the current playback step from state
        int playbackStep = getState().getCurrentIndex();

        // Update grid from APVTS state
        sequenceGrid.updateFromState(getState(), playbackStep);
    }

private:
    SequenceGrid sequenceGrid;
};
