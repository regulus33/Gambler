#pragma once

#include "../PluginProcessor.h"
#include "BinaryData.h"
#include "LogoComponent.h"
#include "NavBarComponent.h"
#include "PianoRollComponent.h"
#include "SequencerStepsComponent.h"
#include "SequencerControlsComponent.h"
#include "MainControlsComponent.h"
#include "SequenceOverviewComponent.h"
#include "SubsequenceControlsComponent.h"
#include "NoteRangeControlsComponent.h"
#include "MusicConstants.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace MainScreenLayout {
    // Background
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;

    // Margins
    constexpr int marginLeft = 22;
    constexpr int marginRight = 22;

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
class MainScreenComponent : public juce::Component,
                            private juce::AudioProcessorValueTreeState::Listener {
public:
    explicit MainScreenComponent(PluginProcessor& processor)
        : processorRef(processor),
          titleFont(juce::FontOptions(
              juce::Typeface::createSystemTypefaceFor(
                  BinaryData::Inter_24ptBold_ttf,
                  BinaryData::Inter_24ptBold_ttfSize))
              .withHeight(MainScreenLayout::titleFontSize)),
          navBar(processor),
          pianoRoll(processor),
          sequencerSteps(processor),
          sequencerControls(processor),
          mainControls(processor),
          sequenceOverview(processor),
          subsequenceControls(processor),
          noteRangeControls(processor)
    {
        // Add navbar
        addAndMakeVisible(navBar);

        // Add logo (above piano roll, left-aligned)
        addAndMakeVisible(logo);

        // Add piano roll component
        addAndMakeVisible(pianoRoll);

        // Initialize piano roll with the global scale
        auto& state = processorRef.getState();
        pianoRoll.setScale(state.getGlobalScale());

        // Setup callback for scale changes - update global scale when user toggles notes
        pianoRoll.onNoteChanged = [this]() {
            // Get the scale from the piano roll
            Scale newScale = pianoRoll.getScale();

            // Update global scale via APVTS-backed state (thread-safe, no command queue needed)
            auto& state = processorRef.getState();
            state.setGlobalScaleNotes(newScale.getScaleNotes());
            DBG("Piano roll scale changed - updated global scale");

            // Update the note controls to only show selected scale notes
            auto selectedIndices = pianoRoll.getSelectedNoteIndices();
            auto selectedNames = pianoRoll.getSelectedNoteNames();
            auto selectedColours = pianoRoll.getSelectedNoteColours();

            // Update note/octave slider options to reflect new scale + current range
            int selectedStep = processorRef.getState().getSelectedStepForEditing();
            int selectedNote = processorRef.getState().getCurrentNoteForStep(selectedStep);
            updateNoteControlOptions(selectedNote);

            // Trigger async update to refresh all UI components with new quantization
            if (onAsyncUpdateNeeded)
                onAsyncUpdateNeeded();
        };

        // Add main controls component (note/octave + probability/grid)
        addAndMakeVisible(mainControls);

        // Initialize note/octave controls with the current scale and range
        {
            int selectedStep = processorRef.getState().getSelectedStepForEditing();
            int selectedNote = processorRef.getState().getCurrentNoteForStep(selectedStep);
            updateNoteControlOptions(selectedNote);
        }

        // Wire up note controls callbacks to update the selected step's note
        mainControls.getNoteControls().onNoteChanged = [this](int pitchClass) {
            // Calculate the new note
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getCurrentNoteForStep(selectedStep);
            int currentOctave = currentNote / 12;
            int newNote = currentOctave * 12 + pitchClass;

            // Update via APVTS-backed state (thread-safe)
            state.updateCurrentNoteForStep(selectedStep, newNote);
            DBG("Note changed to pitch class " << pitchClass << " -> MIDI note " << newNote);

            // Trigger MIDI preview if enabled (read from APVTS)
            if (auto* param = dynamic_cast<juce::AudioParameterBool*>(
                processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_PREVIEW_NOTES))) {
                if (param->get()) {
                    processorRef.triggerCurrentStepMidi(newNote);
                }
            }
        };

        mainControls.getNoteControls().onOctaveChanged = [this](int octaveIndex) {
            // Calculate the new note
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int currentNote = state.getCurrentNoteForStep(selectedStep);
            int pitchClass = currentNote % 12;
            int newNote = octaveIndex * 12 + pitchClass;

            // Update via APVTS-backed state (thread-safe)
            state.updateCurrentNoteForStep(selectedStep, newNote);
            DBG("Octave changed to " << octaveIndex << " -> MIDI note " << newNote);

            // Trigger MIDI preview if enabled (read from APVTS)
            if (auto* param = dynamic_cast<juce::AudioParameterBool*>(
                processorRef.getAPVTS().getParameter(Params::Keys::ID::PREF_PREVIEW_NOTES))) {
                if (param->get()) {
                    processorRef.triggerCurrentStepMidi(newNote);
                }
            }
        };

        // Add sequence overview component
        addAndMakeVisible(sequenceOverview);

        // Initialize the sequence grid to show the first window row (0)
        sequenceOverview.getSequenceGrid().setCurrentWindowRow(0);

        // Setup callback for sequence grid clicks - navigate to the clicked row
        sequenceOverview.getSequenceGrid().onSquareClicked = [this](int stepIndex) {
            // Calculate which row (0-7) the clicked step belongs to
            int rowIndex = stepIndex / 8;
            int windowStart = rowIndex * 8;

            // Navigate to that row
            setSequencerWindow(windowStart);

            // Set the clicked step as the selected step for editing
            auto& state = processorRef.getState();
            state.setSelectedStepForEditing(stepIndex);

            // Update the sequence grid to show the new selection (preserve current step indicator)
            int currentStep = state.getCurrentIndex();
            sequenceOverview.getSequenceGrid().updateFromState(state, currentStep);

            // Update the active step and note
            setActiveStepAndNote(stepIndex);

            DBG("Sequence grid clicked: step " + juce::String(stepIndex) +
                " -> navigating to row " + juce::String(rowIndex) +
                " (window " + juce::String(windowStart) + "), selected for editing");
        };

        // Setup callback for sequence grid double-clicks - force jump to that step
        sequenceOverview.getSequenceGrid().onSquareDoubleClicked = [this](int stepIndex) {
            auto& state = processorRef.getState();
            // Only allow jumping to steps within the sequence length
            if (stepIndex < state.getLength()) {
                state.setCurrentIndex(stepIndex);
                DBG("Sequence grid double-clicked: forcing jump to step " + juce::String(stepIndex));
            }
        };

        // Setup callback for sequence grid drag - change sequence length
        sequenceOverview.getSequenceGrid().onLengthDragged = [this](int newLength) {
            auto& state = processorRef.getState();
            state.setLength(newLength);
            DBG("Sequence grid dragged: new length = " + juce::String(newLength));

            // Update the sequence grid to reflect the new length
            int currentStep = state.getCurrentIndex();
            sequenceOverview.getSequenceGrid().updateFromState(state, currentStep);
        };

        // Add subsequence controls component
        addAndMakeVisible(subsequenceControls);

        setupSubsequenceCallbacks();
        setupSequencerCallbacks();
        setupNoteRangeCallbacks();

        // Listen to UI-specific button params that need window position
        processorRef.getAPVTS().addParameterListener(Params::Keys::ID::INDEX_LEFT, this);
        processorRef.getAPVTS().addParameterListener(Params::Keys::ID::INDEX_RIGHT, this);
        processorRef.getAPVTS().addParameterListener(Params::Keys::ID::CLEAR_WINDOW, this);
    }

    ~MainScreenComponent() override {
        processorRef.getAPVTS().removeParameterListener(Params::Keys::ID::INDEX_LEFT, this);
        processorRef.getAPVTS().removeParameterListener(Params::Keys::ID::INDEX_RIGHT, this);
        processorRef.getAPVTS().removeParameterListener(Params::Keys::ID::CLEAR_WINDOW, this);
    }

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

        // Row 1: Logo (above piano roll) + Navbar (from mainControls x to right edge)
        auto navRow = area.removeFromTop(NavBarConstants::height);
        int navBarX = marginLeft + pianoRollWidth + topRowGap;
        logo.setBounds(navRow.getX() + marginLeft, navRow.getY(),
                        navBarX - marginLeft, NavBarConstants::height);
        navBar.setBounds(navRow.withLeft(navBarX));

        if (compactMode_) {
            // Compact mode: show only one row directly below navbar
            if (showingTopRow_) {
                // Title row
                area.removeFromTop(titleRowHeight);

                // Top content row
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
            } else {
                // Bottom row directly below navbar
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

        if (showingTopRow_)
            return NavBarConstants::height + titleRowHeight + topContentHeight + bottomPadding;
        else
            return NavBarConstants::height + gapAboveSequencer + stepHeight + gapBetweenStepRows + stepHeight + bottomPadding;
    }

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

    // Helper to update the subsequence bar UI from selected step's StepNotes
    void updateSubsequenceBar() {
        auto& state = processorRef.getState();
        int selectedStep = state.getSelectedStepForEditing();

        int length = state.getStepNotesLength(selectedStep);
        int currentSubstepIndex = state.getStepNotesCurrentIndex(selectedStep);

        auto& bar = subsequenceControls.getSubsequenceBar();

        // Update all 8 bars based on the current step's StepNotes
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            if (i < length) {
                // Active substep - color based on note value (quantized for display)
                int midiNote = state.getQuantizedStepNote(selectedStep, i);
                juce::Colour noteColor;
                // Check if note is rest or quantized to rest
                // This happens when: 1) stored note is a rest, or 2) no scale notes are selected
                if (midiNote < 0) {
                    // TODO centralize rest ui presentation
                    noteColor = juce::Colour(SequenceGridConstants::defaultGreyHex);
                } else {
                    int chromaticNote = midiNote % 12;
                    noteColor = state.getNoteColour(chromaticNote);
                }

                bool isCurrentSubstep = (i == currentSubstepIndex);
                bar.setBarColor(i, noteColor);
                bar.setBarBorder(i, isCurrentSubstep);
            } else {
                // Inactive substep - show as default grey (darker)
                bar.setBarColor(i, juce::Colour(SubsequenceControlsConstants::defaultGreyHex));
                bar.setBarBorder(i, false);
            }
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

        // Get the selected step's note for editing controls (may be different from playback position)
        int selectedNote = state.getCurrentNoteForStep(selectedStep);

        // Update the note/octave controls to reflect the selected step's note
        mainControls.getNoteControls().setFromMidiNote(selectedNote);
        mainControls.getNoteControls().setRestOverlay(selectedNote < 0);

        // Update note/octave slider options to reflect current scale + range
        updateNoteControlOptions(selectedNote);

        // Update the sequence overview grid to show all 64 steps
        // - White fill indicates current step (playback position)
        // - White border indicates selected step for editing
        sequenceOverview.getSequenceGrid().updateFromState(state, currentStep);

        // Update the subsequence bar to reflect selected step's substep state
        updateSubsequenceBar();

        // Update the probability gauge to highlight the randomly selected circle (if roll is engaged)
        int lastProbabilityIndex = state.getLastProbabilityIndex();
        auto& probabilityGauge = mainControls.getProbabilityControls().getProbabilityGauge();
        probabilityGauge.highlightCircle(lastProbabilityIndex);

    }

    // Accessors for navbar
    NavBarComponent& getNavBar() { return navBar; }

    // Accessor for note range controls (for MIDI learn repaint)
    NoteRangeControlsComponent& getNoteRangeControls() { return noteRangeControls; }

    // Accessors for testing
    SequenceOverviewComponent& getSequenceOverview() { return sequenceOverview; }
    SubsequenceControlsComponent& getSubsequenceControls() { return subsequenceControls; }

    // Callback for when async update is needed
    std::function<void()> onAsyncUpdateNeeded;

private:
    /**
     * Recompute and push filtered note and octave options to the NoteControlsComponent.
     *
     * Note slider  – shows only pitch classes that have at least one MIDI note within
     *                [rangeMin, rangeMax] AND are enabled in the global scale.
     * Octave slider – shows only octaves where (octave * 12 + currentPitchClass) falls
     *                 within [rangeMin, rangeMax].
     *
     * @param currentMidiNote  The MIDI note currently shown in the controls (used to
     *                         determine the pitch class for octave filtering).
     */
    void updateNoteControlOptions(int currentMidiNote) {
        auto& state = processorRef.getState();
        int rangeMin = state.getScaleRangeMin();
        int rangeMax = state.getScaleRangeMax();

        // --- Note slider: pitch classes reachable in range AND in scale ---
        juce::Array<juce::String> noteNames;
        juce::Array<juce::Colour> noteColours;
        juce::Array<int>          noteIndices;

        for (int pc = 0; pc < 12; ++pc) {
            if (!state.getGlobalScaleNote(pc))
                continue;

            // Check whether any MIDI note with this pitch class falls inside the range
            bool reachable = false;
            for (int midi = pc; midi <= 127; midi += 12) {
                if (midi >= rangeMin && midi <= rangeMax) {
                    reachable = true;
                    break;
                }
            }
            if (!reachable)
                continue;

            noteNames.add(MusicConstants::CHROMATIC_NOTE_NAMES[static_cast<size_t>(pc)]);
            noteColours.add(state.getNoteColour(pc));
            noteIndices.add(pc);
        }

        // Fall back to full scale if nothing is reachable (shouldn't happen in practice)
        if (noteNames.isEmpty()) {
            auto selectedIndices  = pianoRoll.getSelectedNoteIndices();
            auto selectedNames    = pianoRoll.getSelectedNoteNames();
            auto selectedColours  = pianoRoll.getSelectedNoteColours();
            mainControls.getNoteControls().updateNoteOptions(selectedNames, selectedColours, selectedIndices);
        } else {
            mainControls.getNoteControls().updateNoteOptions(noteNames, noteColours, noteIndices);
        }

        // --- Octave slider: octaves where (octave*12 + pitchClass) is in range ---
        int pitchClass = (currentMidiNote >= 0) ? (currentMidiNote % 12) : 0;

        juce::Array<juce::String> octaveLabels;
        juce::Array<juce::Colour> octaveColours;
        juce::Array<int>          octaveIndices;

        for (int oct = 0; oct < 10; ++oct) {
            int midi = oct * 12 + pitchClass;
            if (midi >= rangeMin && midi <= rangeMax) {
                octaveLabels.add(MusicConstants::OCTAVE_LABELS[static_cast<size_t>(oct)]);
                octaveColours.add(MusicConstants::OCTAVE_COLOURS[static_cast<size_t>(oct)]);
                octaveIndices.add(oct);
            }
        }

        // Fall back to all octaves if none are reachable
        if (octaveLabels.isEmpty()) {
            octaveLabels  = MusicConstants::toJuceArray(MusicConstants::OCTAVE_LABELS);
            octaveColours = MusicConstants::toJuceArray(MusicConstants::OCTAVE_COLOURS);
            for (int i = 0; i < 10; ++i) octaveIndices.add(i);
        }

        mainControls.getNoteControls().updateOctaveOptions(octaveLabels, octaveColours, octaveIndices);
    }

    void setupSubsequenceCallbacks() {
        // Wire up MIDI learn registry for subsequence controls
        subsequenceControls.setMidiLearnRegistry(&processorRef.getMidiLearnRegistry());

        // Setup callbacks for subsequence controls
        subsequenceControls.onBarClicked = [this](int index) {
            // Queue command to modify state on audio thread, with completion callback for UI update
            // Update via APVTS-backed state (thread-safe)
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int substepLength = state.getStepNotesLength(selectedStep);

            // Only allow clicking on active substeps (within length)
            if (index < substepLength) {
                state.setStepNotesCurrentIndex(selectedStep, index);
                DBG("Substep " + juce::String(index) + " selected");

                // Update UI
                updateSubsequenceBar();
                int midiNote = state.getCurrentNoteForStep(selectedStep);
                int chromaticNote = midiNote % 12;
                pianoRoll.setActiveNote(chromaticNote);
                mainControls.getNoteControls().setFromMidiNote(midiNote);
                mainControls.getNoteControls().setRestOverlay(midiNote < 0);
            }
        };
        subsequenceControls.onMinusClicked = [this]() {
            // Update via APVTS-backed state (thread-safe)
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int currentLength = state.getStepNotesLength(selectedStep);
            if (currentLength > 1) {
                state.setStepNotesLength(selectedStep, currentLength - 1);
                DBG("Subsequence length decreased to: " + juce::String(currentLength - 1));
                updateSubsequenceBar();
            }
        };
        subsequenceControls.onPlusClicked = [this]() {
            // Update via APVTS-backed state (thread-safe)
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int currentLength = state.getStepNotesLength(selectedStep);
            if (currentLength < ParamConstants::MAX_SUBSTEPS) {
                state.setStepNotesLength(selectedStep, currentLength + 1);
                DBG("Subsequence length increased to: " + juce::String(currentLength + 1));
                updateSubsequenceBar();
            }
        };
    }

    void setupSequencerCallbacks() {
        // Add sequencer steps component
        addAndMakeVisible(sequencerSteps);

        // Wire up MIDI learn registry for step buttons
        sequencerSteps.setRegistry(&processorRef.getMidiLearnRegistry());

        // Setup callback for step clicks - set current index and trigger MIDI
        sequencerSteps.onStepClicked = [this](int stepIndex) {
            // Trigger MIDI for this step (also sets selected step and fires onSequenceTick)
            processorRef.triggerStepMidi(stepIndex);

            // Update subsequence bar directly (triggerStepMidi already set the selected step)
            updateSubsequenceBar();
            DBG("Step clicked: " + juce::String(stepIndex));
        };

        // Add sequencer controls component
        addAndMakeVisible(sequencerControls);

        // Wire up MIDI learn registry for sequencer controls
        sequencerControls.setRegistry(&processorRef.getMidiLearnRegistry());

        // Wire up MIDI learn registry for main controls (note/octave sliders)
        mainControls.setRegistry(&processorRef.getMidiLearnRegistry());

        // Wire up MIDI learn registry for piano roll keys
        pianoRoll.setRegistry(&processorRef.getMidiLearnRegistry());

        // Wire up MIDI learn registry for subdivision slider in navbar
        navBar.getSubdivisionSlider().setRegistry(&processorRef.getMidiLearnRegistry());
        navBar.getSubdivisionSlider().onMidiLearnSelected = [this]() {
            if (onAsyncUpdateNeeded) onAsyncUpdateNeeded();
        };

        // Wire up MIDI learn registry for settings button in navbar
        navBar.setSettingsButtonRegistry(&processorRef.getMidiLearnRegistry());

        // Wire up callback for when a control is selected for MIDI learning
        // This triggers a repaint of all MIDI learn components to update borders
        mainControls.setMidiLearnSelectedCallback([this]() {
            if (onAsyncUpdateNeeded) {
                onAsyncUpdateNeeded();  // Trigger repaint via ApplicationComponent
            }
        });

        // Note: Button actions are now handled via the observer pattern:
        // - Each button sets its APVTS param on mouseDown/mouseUp
        // - PluginProcessor::parameterChanged handles the action
        // - UI-specific actions (PAGE_LEFT, PAGE_RIGHT, CLEAR_WINDOW) are handled
        //   in this component's parameterChanged method
    }

    void setupNoteRangeCallbacks() {
        // Add note range controls component (right of sequencer)
        addAndMakeVisible(noteRangeControls);

        // Wire up MIDI learn registry for note range knobs
        noteRangeControls.setRegistry(&processorRef.getMidiLearnRegistry());
        noteRangeControls.setMidiLearnSelectedCallback([this]() {
            if (onAsyncUpdateNeeded) onAsyncUpdateNeeded();
        });

        // When the range changes, refresh note/octave slider options
        noteRangeControls.onRangeChanged = [this]() {
            auto& state = processorRef.getState();
            int selectedStep = state.getSelectedStepForEditing();
            int selectedNote = state.getCurrentNoteForStep(selectedStep);
            updateNoteControlOptions(selectedNote);
            // Re-display the current note so the slider snaps to the nearest valid option
            mainControls.getNoteControls().setFromMidiNote(selectedNote);
        };

        // Initialize note range controls with current scale window parameters
        noteRangeControls.setWindow(
            processorRef.getState().getScaleWindowOffset(),
            processorRef.getState().getScaleWindowSize());
    }

    /**
     * Set the sequencer window position (which 8 steps are visible)
     * Clamps the window to valid range [0, 56] and only updates if changed.
     * Also updates the SequenceGrid to highlight the current window row.
     *
     * @param windowStart The starting step index (0-56, in increments of 8)
     */
    void setSequencerWindow(int windowStart) {
        // Clamp to valid range (0-56, showing steps 0-7 through 56-63)
        int clampedWindow = juce::jlimit(0, 56, windowStart);

        // Only update if the window needs to change
        auto& state = processorRef.getState();
        if (state.getViewWindowStart() != clampedWindow) {
            state.setViewWindowStart(clampedWindow);

            // Update the sequence grid to highlight the current window row
            int windowRow = clampedWindow / 8;  // Convert window start (0, 8, 16, ..., 56) to row (0-7)
            sequenceOverview.getSequenceGrid().setCurrentWindowRow(windowRow);
        }
    }

    /**
     * Register UI update callbacks for MIDI control
     * These callbacks are invoked when MIDI changes state, to provide visual feedback
     */
    void setupMidiUICallbacks() {
        // Note: Button visual states are now handled automatically via the observer pattern.
        // Each button listens to its own APVTS param and updates its visual state accordingly.
        // MIDI input sets the param, which triggers the button's StateListenerComponent.
        //
        // This method is kept for any future MIDI callbacks that may be needed,
        // but the button-related callbacks have been removed as they're now redundant.
        juce::ignoreUnused(processorRef);
    }

    // ========================================
    // AudioProcessorValueTreeState::Listener implementation
    // ========================================

    /**
     * Handle UI-specific button params that need window position.
     * Called from any thread when param changes - use MessageManager for UI updates.
     */
    void parameterChanged(const juce::String& parameterID, float newValue) override {
        bool isOn = newValue > 0.5f;

        // Only act on the ON transition (button press)
        if (!isOn) return;

        // Use MessageManager to safely update UI from any thread
        juce::MessageManager::callAsync([this, parameterID]() {
            auto& state = processorRef.getState();

            if (parameterID == Params::Keys::ID::INDEX_LEFT) {
                int currentWindow = state.getViewWindowStart();
                int newWindow = currentWindow - 8;
                setSequencerWindow(newWindow);
                DBG("Page left, window now at: " + juce::String(state.getViewWindowStart()));
            }
            else if (parameterID == Params::Keys::ID::INDEX_RIGHT) {
                int currentWindow = state.getViewWindowStart();
                int newWindow = currentWindow + 8;
                setSequencerWindow(newWindow);
                DBG("Page right, window now at: " + juce::String(state.getViewWindowStart()));
            }
            else if (parameterID == Params::Keys::ID::CLEAR_WINDOW) {
                // Clear: set all steps in current window to rest
                int windowStart = state.getViewWindowStart();
                for (int i = 0; i < 8; i++) {
                    int stepIdx = windowStart + i;
                    for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                        state.setStepNote(stepIdx, n, -1);
                    }
                }
                sequencerSteps.repaint();
                DBG("Cleared window starting at " + juce::String(windowStart));
            }
        });
    }

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

    NavBarComponent navBar;
    LogoComponent logo;
    PianoRollComponent pianoRoll;
    SequencerStepsComponent sequencerSteps;
    SequencerControlsComponent sequencerControls;
    MainControlsComponent mainControls;
    SequenceOverviewComponent sequenceOverview;
    SubsequenceControlsComponent subsequenceControls;
    NoteRangeControlsComponent noteRangeControls;
};


