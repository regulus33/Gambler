#pragma once

#include "../PluginProcessor.h"
#include "BinaryData.h"
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
    constexpr int subsequenceExtraGap = 22;  // extra gap before subsequence

    // Top content row height (tallest panel determines the row height)
    constexpr int topContentHeight = 225;  // sequence grid (196) + padding

    // Sequencer rows
    constexpr int gapAboveSequencer = 20;
    constexpr int sequencerWidth = 862;
    constexpr int stepHeight = 92;
    constexpr int gapBetweenStepRows = 14;
    constexpr int noteRangeGap = 18;
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

        // Add piano roll component
        addAndMakeVisible(pianoRoll);

        // Initialize piano roll with the selected step's scale
        auto& state = processorRef.getState();
        int selectedStep = state.getSelectedStepForEditing();
        if (selectedStep >= 0) {
            pianoRoll.setScale(state.getStepScale(selectedStep));
        }

        // Setup callback for scale changes - update selected step's scale when user toggles notes
        pianoRoll.onNoteChanged = [this]() {
            // Get the scale from the piano roll
            Scale newScale = pianoRoll.getScale();

            // Update scale via APVTS-backed state (thread-safe, no command queue needed)
            auto& state = processorRef.getState();

            // Update the selected step's scale
            int selectedStep = state.getSelectedStepForEditing();
            if (selectedStep >= 0) {
                state.setStepScaleNotes(selectedStep, newScale.getScaleNotes());
                DBG("Piano roll scale changed - updated step: " << selectedStep << " scale");
            }

            // Update the note controls to only show selected scale notes
            auto selectedIndices = pianoRoll.getSelectedNoteIndices();
            auto selectedNames = pianoRoll.getSelectedNoteNames();
            auto selectedColours = pianoRoll.getSelectedNoteColours();

            // Update the note slider options
            mainControls.getNoteControls().updateNoteOptions(selectedNames, selectedColours, selectedIndices);

            // Trigger async update to refresh all UI components with new quantization
            if (onAsyncUpdateNeeded)
                onAsyncUpdateNeeded();
        };

        // Add main controls component (note/octave + probability/grid)
        addAndMakeVisible(mainControls);

        // Initialize note controls with the current scale
        {
            auto selectedIndices = pianoRoll.getSelectedNoteIndices();
            auto selectedNames = pianoRoll.getSelectedNoteNames();
            auto selectedColours = pianoRoll.getSelectedNoteColours();
            mainControls.getNoteControls().updateNoteOptions(selectedNames, selectedColours, selectedIndices);
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

            // Update the piano roll to show the selected step's scale
            Scale stepScale = processorRef.getState().getStepScale(stepIndex);
            pianoRoll.setScale(stepScale);

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

        // Draw section titles in the title row (above their respective panels)
        g.setFont(titleFont);
        g.setColour(juce::Colours::white);

        auto titleRect = [&](const juce::Component& comp) {
            return juce::Rectangle<int>(comp.getX(), comp.getY() - titleRowHeight, comp.getWidth(), titleRowHeight);
        };
        g.drawText("Main Controls", titleRect(mainControls), juce::Justification::centred, false);
        g.drawText("Sequence", titleRect(sequenceOverview), juce::Justification::centred, false);
        g.drawText("Subseq", titleRect(subsequenceControls), juce::Justification::centred, false);
    }

    void resized() override {
        using namespace MainScreenLayout;

        auto area = getLocalBounds();

        // Row 1: Navbar
        navBar.setBounds(area.removeFromTop(NavBarConstants::height));

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

    // Accessors for testing
    MainControlsComponent& getMainControls() { return mainControls; }
    PianoRollComponent& getPianoRoll() { return pianoRoll; }
    SequencerControlsComponent& getSequencerControls() { return sequencerControls; }
    SequencerStepsComponent& getSequencerSteps() { return sequencerSteps; }

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

        // Note: WriteScale is now handled in audio thread before quantization (see PluginProcessor::processBlock)

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

        // Update the piano roll to show the selected step's scale
        if (selectedStep >= 0) {
            Scale stepScale = state.getStepScale(selectedStep);
            pianoRoll.setScale(stepScale);
        }
    }

    // Accessors for navbar
    NavBarComponent& getNavBar() { return navBar; }

    // Accessor for note range controls (for MIDI learn repaint)
    NoteRangeControlsComponent& getNoteRangeControls() { return noteRangeControls; }

    // Accessors for testing
    SequenceOverviewComponent& getSequenceOverview() { return sequenceOverview; }

    // Callback for when async update is needed
    std::function<void()> onAsyncUpdateNeeded;

private:
    void setupSubsequenceCallbacks() {
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

    PluginProcessor& processorRef;
    juce::Font titleFont;

    NavBarComponent navBar;
    PianoRollComponent pianoRoll;
    SequencerStepsComponent sequencerSteps;
    SequencerControlsComponent sequencerControls;
    MainControlsComponent mainControls;
    SequenceOverviewComponent sequenceOverview;
    SubsequenceControlsComponent subsequenceControls;
    NoteRangeControlsComponent noteRangeControls;
};


