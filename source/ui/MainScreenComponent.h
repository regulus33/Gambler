#pragma once

#include "../PluginProcessor.h"
#include "BinaryData.h"
#include "NavBarComponent/NavBarComponent.h"
#include "PianoRollComponent/PianoRollComponent.h"
#include "SequencerStepsComponent.h"
#include "SequencerControlsComponent.h"
#include "MainControlsComponent.h"
#include "SequenceOverviewComponent.h"
#include "SubsequencerComponent/SubsequenceControlsComponent.h"
#include "NoteRangeControlsComponent/NoteRangeControlsComponent.h"
#include "../data/Params.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace MainScreenLayout {
    // Background
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;

    // Margins
    constexpr int marginLeft = 22;
    constexpr int marginRight = 22;
    constexpr int presetSelectorMarginRight = 38;  // gap between preset selector and window edge

    // Title row (between navbar and content)
    constexpr int titleRowHeight = 25;
    constexpr float titleFontSize = 16.0f;

    // Top content row
    constexpr int pianoRollWidth = 422;
    constexpr int pianoRollHeight = 206;
    constexpr int mainControlsWidth = 198;
    constexpr int sequenceOverviewWidth = 198;
    constexpr int subsequenceControlsWidth = SubsequenceControlsConstants::containerWidth;
    constexpr int topRowGap = 18;            // horizontal gap between panels
    constexpr int subsequenceExtraGap = 32;  // hack to make it look centered over note range conrols.

    // Top content row height (tallest panel determines the row height)
    constexpr int topContentHeight = 225;  // sequence grid (196) + padding

    // Sequencer rows
    constexpr int gapAboveSequencer = 0;
    constexpr int sequencerWidth = 862;
    constexpr int stepHeight = 92;
    constexpr int gapBetweenStepRows = 14;
    constexpr int noteRangeGap = 18;
    constexpr int bottomPadding = 24;   // padding below sequencer row / bottom of window

    // Compact top row: reduce top content height (less bottom padding below piano roll)
    constexpr int compactTopContentHeight = 210;

    // Compact mode shared layout — both compact rows target the same ideal
    // window height so swapping modes doesn't resize the host. Driven by the
    // compact-top row's minimum needs.
    constexpr int compactTopContentGap    = 15;  // gap between navbar and content (compact top row)
    constexpr int compactBottomContentGap = 15;  // gap between navbar and content (compact bottom row)
    constexpr int compactBottomPadding    = 32;  // breathing room below content (compact top)
    // = 45 (navbar) + 15 (gap) + 25 (titles) + 210 (content) + 32 (bottom) = 327
    constexpr int compactIdealHeight =
        NavBarConstants::height + compactTopContentGap + titleRowHeight
        + compactTopContentHeight + compactBottomPadding;
}

/**
 * MainScreenComponent
 *
 * The main sequencer screen that displays:
 * - NavBarComponent at the top
 * - Piano roll / MIDI data view
 * - Sequencer steps and controls
 * - Main controls (note/octave, probability, grid)
 * - Sequence overview
 * - Subsequence controls
 * - Note range controls
 *
 * This component handles all the main sequencer UI and callbacks.
 *
 * It also listens to certain APVTS parameters that require UI-specific handling:
 * - INDEX_LEFT/INDEX_RIGHT: Page the view window (needs window position from UI)
 * - CLEAR_WINDOW: Clear steps in current window (needs window position from UI)
 */
class MainScreenComponent : public juce::Component {
public:
    explicit MainScreenComponent(PluginProcessor& processor)
        : processorRef(processor),
          titleFont(juce::FontOptions(
              juce::Typeface::createSystemTypefaceFor(
                  BinaryData::Inter_24ptBold_ttf,
                  BinaryData::Inter_24ptBold_ttfSize))
              .withHeight(MainScreenLayout::titleFontSize)),
          pianoRoll(processor),
          sequencerSteps(processor),
          sequencerControls(processor),
          mainControls(processor),
          sequenceOverview(processor),
          subsequenceControls(processor),
          noteRangeControls(processor)
    {
        // Add piano roll component
        addAndMakeVisible(pianoRoll);

        // Initialize piano roll with the global scale
        auto& state = processorRef.getState();
        pianoRoll.setScale(state.getGlobalScale());

        // Add main controls component (note/octave + probability/grid)
        addAndMakeVisible(mainControls);

        // Add sequence overview component
        addAndMakeVisible(sequenceOverview);

        // Initialize the sequence grid to show the first window row (0)
        sequenceOverview.getSequenceGrid().setCurrentWindowRow(0);

        // Add subsequence controls component
        addAndMakeVisible(subsequenceControls);

        // Add sequencer steps and controls
        addAndMakeVisible(sequencerSteps);
        addAndMakeVisible(sequencerControls);

        // Add note range controls
        addAndMakeVisible(noteRangeControls);
    }

    ~MainScreenComponent() override = default;

    void paint(juce::Graphics& g) override {
        using namespace MainScreenLayout;
        g.fillAll(juce::Colour(bgColourHex));

        // Draw section titles only when top row panels are visible
        if (!compactMode_ || showingTopRow_) {
            g.setFont(titleFont);
            g.setColour(juce::Colours::white);

            auto titleRect = [&](const juce::Component& comp) {
                return juce::Rectangle<int>(comp.getX(), comp.getY() - titleRowHeight, comp.getWidth(), titleRowHeight);
            };
            g.drawText("Main Controls", titleRect(mainControls), juce::Justification::centred, false);
            g.drawText("Sequence", titleRect(sequenceOverview), juce::Justification::centred, false);
            g.drawText("Subseq", titleRect(subsequenceControls), juce::Justification::centred, false);
        }
    }

    void resized() override {
        using namespace MainScreenLayout;

        auto area = getLocalBounds();

        // Row 1: Reserve navbar height (logo + navbar owned by ApplicationComponent)
        area.removeFromTop(NavBarConstants::height);

        if (compactMode_) {
            // Compact mode: each row has its own gap below the navbar so they
            // can be tuned independently.
            area.removeFromTop(showingTopRow_ ? compactTopContentGap : compactBottomContentGap);

            if (showingTopRow_) {
                // Title row reserves its own height above the content block.
                area.removeFromTop(titleRowHeight);

                auto topRow = area.removeFromTop(compactTopContentHeight);
                topRow.removeFromLeft(marginLeft);
                topRow.removeFromRight(marginRight);

                pianoRoll.setBounds(topRow.removeFromLeft(pianoRollWidth).withHeight(pianoRollHeight));
                topRow.removeFromLeft(topRowGap);
                mainControls.setBounds(topRow.removeFromLeft(mainControlsWidth));
                topRow.removeFromLeft(topRowGap);
                sequenceOverview.setBounds(topRow.removeFromLeft(sequenceOverviewWidth));
                topRow.removeFromLeft(subsequenceExtraGap);
                subsequenceControls.setBounds(topRow.removeFromLeft(subsequenceControlsWidth));
            } else {
                auto bottomArea = area.removeFromTop(stepHeight * 2 + gapBetweenStepRows);
                bottomArea.removeFromLeft(marginLeft);
                bottomArea.removeFromRight(marginRight);

                auto sequencerArea = bottomArea.removeFromLeft(sequencerWidth);
                bottomArea.removeFromLeft(noteRangeGap);
                auto noteRangeArea = bottomArea;

                sequencerSteps.setBounds(sequencerArea.removeFromTop(stepHeight));
                sequencerArea.removeFromTop(gapBetweenStepRows);
                sequencerControls.setBounds(sequencerArea.removeFromTop(stepHeight));

                noteRangeControls.setBounds(noteRangeArea.withHeight(stepHeight * 2 + gapBetweenStepRows));
            }
        } else {
            // Full view: both rows visible

            // Row 2: Title row (just reserves space — titles drawn in paint)
            area.removeFromTop(titleRowHeight);

            // Row 3: Top content (piano roll + panels)
            auto topRow = area.removeFromTop(topContentHeight);
            topRow.removeFromLeft(marginLeft);
            topRow.removeFromRight(marginRight);

            pianoRoll.setBounds(topRow.removeFromLeft(pianoRollWidth).withHeight(pianoRollHeight));
            topRow.removeFromLeft(topRowGap);
            mainControls.setBounds(topRow.removeFromLeft(mainControlsWidth));
            topRow.removeFromLeft(topRowGap);
            sequenceOverview.setBounds(topRow.removeFromLeft(sequenceOverviewWidth));
            topRow.removeFromLeft(subsequenceExtraGap);
            subsequenceControls.setBounds(topRow.removeFromLeft(subsequenceControlsWidth));

            // Row 4 & 5: Sequencer steps + controls with note range on the right
            area.removeFromTop(gapAboveSequencer);

            auto bottomArea = area;
            bottomArea.removeFromLeft(marginLeft);
            bottomArea.removeFromRight(marginRight);

            auto sequencerArea = bottomArea.removeFromLeft(sequencerWidth);
            bottomArea.removeFromLeft(noteRangeGap);
            auto noteRangeArea = bottomArea;

            sequencerSteps.setBounds(sequencerArea.removeFromTop(stepHeight));
            sequencerArea.removeFromTop(gapBetweenStepRows);
            sequencerControls.setBounds(sequencerArea.removeFromTop(stepHeight));

            noteRangeControls.setBounds(noteRangeArea.withHeight(stepHeight * 2 + gapBetweenStepRows));
        }
    }

    // Accessors for testing
    MainControlsComponent& getMainControls() { return mainControls; }
    PianoRollComponent& getPianoRoll() { return pianoRoll; }
    SequencerControlsComponent& getSequencerControls() { return sequencerControls; }
    SequencerStepsComponent& getSequencerSteps() { return sequencerSteps; }

    /**
     * Set compact view mode — shows only one row at a time
     * @param compact true to enter compact mode, false for full view
     */
    void setCompactMode(bool compact) {
        if (compactMode_ == compact) return;
        compactMode_ = compact;
        if (!compact) showingTopRow_ = false; // reset to bottom row default
        updateRowVisibility();
        resized();
        repaint();
        if (onIdealHeightChanged) onIdealHeightChanged(getIdealHeight());
    }

    /**
     * In compact mode, swap which row is visible
     * @param showTop true = show top row (piano roll, main controls, etc.), false = show bottom row (sequencer)
     */
    void setShowingTopRow(bool showTop) {
        if (showingTopRow_ == showTop) return;
        showingTopRow_ = showTop;
        updateRowVisibility();
        resized();
        repaint();
        if (onIdealHeightChanged) onIdealHeightChanged(getIdealHeight());
    }

    /**
     * Get the ideal design height for the current view mode
     */
    int getIdealHeight() const {
        using namespace MainScreenLayout;
        if (!compactMode_)
            return fullDesignHeight;

        // Both compact modes share the same ideal height so toggling rows
        // never resizes the host window.
        return compactIdealHeight;
    }

    bool isCompactMode() const { return compactMode_; }
    bool isShowingTopRow() const { return showingTopRow_; }

    // Full design height: navbar + title + top content + gap + 2 step rows + gap + bottom padding
    static constexpr int fullDesignHeight = NavBarConstants::height + MainScreenLayout::titleRowHeight
        + MainScreenLayout::topContentHeight + MainScreenLayout::gapAboveSequencer
        + MainScreenLayout::stepHeight + MainScreenLayout::gapBetweenStepRows
        + MainScreenLayout::stepHeight + MainScreenLayout::bottomPadding;

    // Callback when ideal height changes (for window resizing)
    std::function<void(int)> onIdealHeightChanged;

    // Helper to set active step and update piano roll
    void setActiveStepAndNote(int stepIndex) {
        if (stepIndex >= 0 && stepIndex < 64) {
            // Get the note from this step in the sequence state
            int midiNote = processorRef.getState().getCurrentNoteForStep(stepIndex);

            // Calculate chromatic note (0-11) for piano roll
            int chromaticNote = midiNote % 12;

            // Update the piano roll active note
            pianoRoll.setActiveNote(chromaticNote);

        } else {
            // Clear both if stepIndex is -1
            pianoRoll.setActiveNote(-1);
        }
    }

    // Update UI from sequence state (called from async update)
    void updateFromSequence() {
        auto& state = processorRef.getState();
        int currentStep = state.getCurrentIndex();
        int selectedStep = state.getSelectedStepForEditing();

        // Update the active step indicator to follow the sequence playback
        setActiveStepAndNote(currentStep);

        // Auto-scroll the sequencer window to follow the active step (if enabled, read from APVTS)
        if (auto* param = dynamic_cast<juce::AudioParameterBool*>(
            processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_FOLLOW_STEP))) {
            if (param->get() && currentStep >= 0) {
                // Calculate which "row" (0-7) the current step is in (8 steps per row)
                int rowIndex = currentStep / 8;
                int windowStart = rowIndex * 8;
                setSequencerWindow(windowStart);
            }
        }

        // Repaint the sequencer steps to show updated note values
        sequencerSteps.repaint();

        // Repaint the sequencer controls to show updated MIDI mappings
        sequencerControls.repaint();

        // Update the note range gauge to highlight the current playing note
        int currentNote = state.getCurrentNoteForStep(currentStep);
        noteRangeControls.setActiveNote(currentNote);

        // Get the selected substep's note for editing controls
        int selectedNote = state.getSelectedNoteForStep(selectedStep);

        // Update the note/octave controls to reflect the selected substep's note
        mainControls.getNoteControls().setFromMidiNote(selectedNote);
        mainControls.getNoteControls().setRestOverlay(selectedNote < 0);

        // Note: updateNoteControlOptions is called by ApplicationComponent

        // Update the sequence overview grid to show all 64 steps
        // - White fill indicates current step (playback position)
        // - White border indicates selected step for editing
        sequenceOverview.getSequenceGrid().updateFromState(state, currentStep);

        // Update the probability gauge to highlight the randomly selected circle (if roll is engaged)
        int lastProbabilityIndex = state.getLastProbabilityIndex();
        auto& probabilityGauge = mainControls.getProbabilityControls().getProbabilityGauge();
        probabilityGauge.highlightCircle(lastProbabilityIndex);

    }

    // Accessor for note range controls
    NoteRangeControlsComponent& getNoteRangeControls() { return noteRangeControls; }

    // Accessors
    SequenceOverviewComponent& getSequenceOverview() { return sequenceOverview; }
    SubsequenceControlsComponent& getSubsequenceControls() { return subsequenceControls; }

    void setSequencerWindow(int windowStart) {
        int clampedWindow = juce::jlimit(0, 56, windowStart);
        auto& state = processorRef.getState();
        if (state.getViewWindowStart() != clampedWindow) {
            state.setViewWindowStart(clampedWindow);
            int windowRow = clampedWindow / 8;
            sequenceOverview.getSequenceGrid().setCurrentWindowRow(windowRow);
        }
    }

private:
    void updateRowVisibility() {
        if (!compactMode_) {
            // Full view — everything visible
            pianoRoll.setVisible(true);
            mainControls.setVisible(true);
            sequenceOverview.setVisible(true);
            subsequenceControls.setVisible(true);
            sequencerSteps.setVisible(true);
            sequencerControls.setVisible(true);
            noteRangeControls.setVisible(true);
        } else if (showingTopRow_) {
            // Compact mode showing top row
            pianoRoll.setVisible(true);
            mainControls.setVisible(true);
            sequenceOverview.setVisible(true);
            subsequenceControls.setVisible(true);
            sequencerSteps.setVisible(false);
            sequencerControls.setVisible(false);
            noteRangeControls.setVisible(false);
        } else {
            // Compact mode showing bottom row (default)
            pianoRoll.setVisible(false);
            mainControls.setVisible(false);
            sequenceOverview.setVisible(false);
            subsequenceControls.setVisible(false);
            sequencerSteps.setVisible(true);
            sequencerControls.setVisible(true);
            noteRangeControls.setVisible(true);
        }
    }

    bool compactMode_ = false;
    bool showingTopRow_ = false;

    PluginProcessor& processorRef;
    juce::Font titleFont;

    PianoRollComponent pianoRoll;
    SequencerStepsComponent sequencerSteps;
    SequencerControlsComponent sequencerControls;
    MainControlsComponent mainControls;
    SequenceOverviewComponent sequenceOverview;
    SubsequenceControlsComponent subsequenceControls;
    NoteRangeControlsComponent noteRangeControls;
};


