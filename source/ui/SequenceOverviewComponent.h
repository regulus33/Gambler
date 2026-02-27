#pragma once

#include "BinaryData.h"
#include "SequenceGrid.h"
#include "StateListenerComponent.h"
#include "UIConstants.h"
#include "../backend/MidiExporter.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace SequenceOverviewConstants {
    // Title
    constexpr float titleFontSize = 16.0f;
    constexpr int titleHeight = 24;
    constexpr int gapBelowTitle = 13;
    // Container layout
    constexpr int containerWidth = 198;
    // Grid positioning (centered within container)
    constexpr int gridHorizontalPadding = (containerWidth - SequenceGridConstants::gridWidth) / 2;
}

/**
 * SequenceOverviewComponent
 *
 * Top-level container for the Sequence Overview UI.
 * - Title "Sequence" at the top
 * - SequenceGrid (8x8 grid of colored squares)
 *
 * Observer Pattern:
 * - Listens to all step note parameters and sequence metadata
 * - Automatically updates the SequenceGrid when state changes
 */
class SequenceOverviewComponent : public StateListenerComponent {
public:
    explicit SequenceOverviewComponent(PluginProcessor& processor)
        : StateListenerComponent(processor),
          titleFont (juce::FontOptions (
              juce::Typeface::createSystemTypefaceFor (
                  BinaryData::Inter_24ptBold_ttf,
                  BinaryData::Inter_24ptBold_ttfSize))
              .withHeight (SequenceOverviewConstants::titleFontSize))
    {
        addAndMakeVisible (sequenceGrid);

        // Subscribe to all step notes (for color updates)
        listenToStepNotes(0, 63);
        listenToStepNotesLength(0, 63);

        // Subscribe to sequence metadata (for length, current index, selected step)
        listenToSequenceMetadata();
    }

    void paint (juce::Graphics& g) override {
        using namespace SequenceOverviewConstants;
        g.fillAll (juce::Colours::transparentBlack);

        g.setFont (titleFont);
        g.setColour (juce::Colours::white);

        auto area = getLocalBounds();
        auto titleArea = area.removeFromTop (titleHeight);
        titleArea.translate (0, UIConstants::titleYOffset);
        g.drawText ("Sequence", titleArea, juce::Justification::centred, false);
    }

    void resized() override {
        using namespace SequenceOverviewConstants;

        int gridY = titleHeight + gapBelowTitle;

        // Center the grid horizontally
        sequenceGrid.setBounds (
            gridHorizontalPadding,
            gridY,
            SequenceGridConstants::gridWidth,
            SequenceGridConstants::gridHeight);
    }

    // Access to the grid for external updates
    SequenceGrid& getSequenceGrid() { return sequenceGrid; }
    const SequenceGrid& getSequenceGrid() const { return sequenceGrid; }

    void mouseDown(const juce::MouseEvent&) override {
        dragStarted = false;
    }

    void mouseDrag(const juce::MouseEvent& event) override {
        if (!dragStarted && event.getDistanceFromDragStart() > 10) {
            dragStarted = true;
            auto midiFile = MidiExporter::createMidiFile(getState());
            auto tempFile = MidiExporter::writeToTempFile(midiFile);
            juce::DragAndDropContainer::performExternalDragDropOfFiles(
                {tempFile.getFullPathName()}, false, this);
        }
    }

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
    juce::Font titleFont;
    SequenceGrid sequenceGrid;
    bool dragStarted = false;
};

