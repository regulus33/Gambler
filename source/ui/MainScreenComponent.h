#pragma once

#include "../PluginProcessor.h"
#include "BinaryData.h"
#include "NavBarComponent.h"
#include "PianoRollComponent.h"
#include "MidiDataPerStepSelectionComponent.h"
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
    // Piano roll / MIDI data panel
    constexpr int pianoRollX = 22;
    constexpr int pianoRollY = 95;
    constexpr int pianoRollWidth = 422;
    constexpr int pianoRollHeight = 206;
    // Main controls
    constexpr int mainControlsOverflowTop = 35;
    constexpr int mainControlsOverflowBottom = 0;
    constexpr int mainControlsGapFromPianoRoll = 18;
    constexpr int mainControlsWidth = 198;
    // Sequence overview
    constexpr int sequenceOverviewGapFromMainControls = 18;
    constexpr int sequenceOverviewWidth = 198;
    constexpr int sequenceOverviewHeight = 260;
    // Subsequence controls
    constexpr int subsequenceControlsGap = 40;
    constexpr int subsequenceControlsWidth = SubsequenceControlsConstants::containerWidth;
    constexpr int subsequenceControlsHeight = 260;
    // Sequencer
    constexpr int sequencerX = 22;
    constexpr int sequencerStepsY = 312;
    constexpr int sequencerWidth = 862;
    constexpr int sequencerStepsHeight = 92;
    constexpr int sequencerControlsY = 422;
    constexpr int sequencerControlsHeight = 92;
    // Note range controls (right of sequencer)
    constexpr int noteRangeControlsGap = 18;
    constexpr int noteRangeControlsRightMargin = 22;
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
 */
class MainScreenComponent : public juce::Component {
public:
    explicit MainScreenComponent(PluginProcessor& processor)
        : processorRef(processor),
          pianoRoll(processor),
          midiDataPerStep(processor),
          sequencerSteps(processor),
          sequencerControls(processor),
          mainControls(processor),
          sequenceOverview(processor),
          subsequenceControls(processor),
          noteRangeControls(processor)
    {
        // Add navbar
        addAndMakeVisible(navBar);

        // Setup callback for view toggle button in navbar
        navBar.onViewToggleClicked = [this](bool toggled) {
            // toggled = true means icon 2 (MIDI data view), false means icon 1 (piano roll view)
            bool showPianoRoll = !toggled;
            pianoRoll.setVisible(showPianoRoll);
            midiDataPerStep.setVisible(!showPianoRoll);
            DBG("View mode changed: " + juce::String(showPianoRoll ? "PianoRoll" : "MIDI Data"));
        };

        // Add piano roll component (visible by default)
        addAndMakeVisible(pianoRoll);

        // Initialize piano roll with the current scale from the state
        pianoRoll.setScale(processorRef.getState().getScale());

        // Setup callback for scale changes - update backend Scale when user toggles notes
        pianoRoll.onNoteChanged = [this]() {
            // Get the scale from the piano roll
            Scale newScale = pianoRoll.getScale();

            // Update scale via APVTS-backed state (thread-safe, no command queue needed)
            auto& state = processorRef.getState();
            state.setScaleNotes(newScale.getScaleNotes());
            state.quantizeAllNotesToScale();
            DBG("Piano roll scale changed - updated backend Scale and quantized all notes");

            // Update the note controls to only show selected scale notes
            auto selectedIndices = pianoRoll.getSelectedNoteIndices();
            auto selectedNames = pianoRoll.getSelectedNoteNames();
            auto selectedColours = pianoRoll.getSelectedNoteColours();

            // Update the note slider options
            mainControls.getNoteControls().updateNoteOptions(selectedNames, selectedColours, selectedIndices);

            // Trigger async update to refresh all UI components after quantization
            if (onAsyncUpdateNeeded)
                onAsyncUpdateNeeded();
        };

        // Add MIDI data per step component (hidden by default)
        addChildComponent(midiDataPerStep);

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

            // Trigger MIDI preview if enabled
            bool previewEnabled = processorRef.getUserPreferences().previewNotesOnEdit;
            if (previewEnabled) {
                processorRef.triggerCurrentStepMidi(newNote);
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

            // Trigger MIDI preview if enabled
            bool previewEnabled = processorRef.getUserPreferences().previewNotesOnEdit;
            if (previewEnabled) {
                processorRef.triggerCurrentStepMidi(newNote);
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
            processorRef.getState().setSelectedStepForEditing(stepIndex);
            sequencerSteps.setSelectedStep(stepIndex);

            // Update the sequence grid to show the new selection
            sequenceOverview.getSequenceGrid().updateFromState(processorRef.getState());

            // Update the MIDI data per step component to show the selected step's data
            midiDataPerStep.updateFromSequence();

            DBG("Sequence grid clicked: step " + juce::String(stepIndex) +
                " -> navigating to row " + juce::String(rowIndex) +
                " (window " + juce::String(windowStart) + "), selected for editing");
        };

        // Add subsequence controls component
        addAndMakeVisible(subsequenceControls);

        setupSubsequenceCallbacks();
        setupSequencerCallbacks();
        setupNoteRangeCallbacks();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(MainScreenLayout::bgColourHex));
    }

    void resized() override {
        using namespace MainScreenLayout;

        auto area = getLocalBounds();

        // Position navbar at the top
        navBar.setBounds(area.removeFromTop(NavBarConstants::height));

        // Position piano roll
        pianoRoll.setBounds(pianoRollX, pianoRollY, pianoRollWidth, pianoRollHeight);

        // Position MIDI data per step component at same location as piano roll
        midiDataPerStep.setBounds(pianoRollX, pianoRollY, pianoRollWidth, pianoRollHeight);

        // Position main controls to the right of the piano roll / MIDI data panel
        int mainControlsX = pianoRollX + pianoRollWidth + mainControlsGapFromPianoRoll;
        int mainControlsY = pianoRollY - mainControlsOverflowTop;
        int mainControlsHeight = pianoRollHeight + mainControlsOverflowTop + mainControlsOverflowBottom;
        mainControls.setBounds(mainControlsX, mainControlsY, mainControlsWidth, mainControlsHeight);

        // Position sequence overview to the right of main controls
        int sequenceOverviewX = mainControlsX + mainControlsWidth + sequenceOverviewGapFromMainControls;
        int sequenceOverviewY = mainControlsY;
        sequenceOverview.setBounds(sequenceOverviewX, sequenceOverviewY, sequenceOverviewWidth, sequenceOverviewHeight);

        // Position subsequence controls to the right of sequence overview
        int subsequenceControlsX = sequenceOverviewX + sequenceOverviewWidth + subsequenceControlsGap;
        int subsequenceControlsY = mainControlsY;
        subsequenceControls.setBounds(subsequenceControlsX, subsequenceControlsY, subsequenceControlsWidth, subsequenceControlsHeight);

        // Position sequencer steps
        sequencerSteps.setBounds(sequencerX, sequencerStepsY, sequencerWidth, sequencerStepsHeight);

        // Position sequencer controls
        sequencerControls.setBounds(sequencerX, sequencerControlsY, sequencerWidth, sequencerControlsHeight);

        // Position note range controls (right of sequencer)
        int noteRangeControlsX = sequencerX + sequencerWidth + noteRangeControlsGap;
        int noteRangeControlsY = sequencerStepsY;
        int noteRangeControlsWidth = getWidth() - noteRangeControlsX - noteRangeControlsRightMargin;
        int noteRangeControlsHeight = (sequencerControlsY - sequencerStepsY) + sequencerControlsHeight;
        noteRangeControls.setBounds(noteRangeControlsX, noteRangeControlsY, noteRangeControlsWidth, noteRangeControlsHeight);
    }

    // Helper to set active step and update piano roll
    void setActiveStepAndNote(int stepIndex) {
        // Update the active step in the sequencer steps component
        sequencerSteps.setActiveStep(stepIndex);

        if (stepIndex >= 0 && stepIndex < 64) {
            // Get the note from this step in the sequence state
            int midiNote = processorRef.getState().getCurrentNoteForStep(stepIndex);

            // Calculate chromatic note (0-11) for piano roll
            int chromaticNote = midiNote % 12;

            // Update the piano roll active note
            pianoRoll.setActiveNote(chromaticNote);

            // Update MIDI data per step component with current step's modulations
            midiDataPerStep.updateFromSequence();
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
                // Active substep - color based on note value
                int midiNote = state.getStepNote(selectedStep, i);
                int chromaticNote = midiNote % 12;
                juce::Colour noteColor = MusicConstants::NOTE_COLOURS[chromaticNote];
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

        // If followPlayback is enabled, update the selected step to match playback position
        if (processorRef.getUserPreferences().followPlayback && currentStep >= 0) {
            state.setSelectedStepForEditing(currentStep);
            selectedStep = currentStep; // Update local copy for UI rendering
        }

        // Update the active step indicator to follow the sequence playback
        setActiveStepAndNote(currentStep);

        // Update the selected step indicator for editing
        sequencerSteps.setSelectedStep(selectedStep);

        // Auto-scroll the sequencer window to follow the active step (if enabled)
        if (processorRef.getUserPreferences().followStep && currentStep >= 0) {
            // Calculate which "row" (0-7) the current step is in (8 steps per row)
            int rowIndex = currentStep / 8;
            int windowStart = rowIndex * 8;
            setSequencerWindow(windowStart);
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

        // Update the sequence overview grid to show all 64 steps
        // - Border indicates playback position (currentStep)
        // - White fill indicates selected step for editing (selectedStep)
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

        // Setup callback for step clicks - set current index and trigger MIDI
        sequencerSteps.onStepClicked = [this](int stepIndex) {
            // Trigger MIDI for this step (also sets current index via queued command)
            processorRef.triggerStepMidi(stepIndex);

            // Queue a separate command with completion callback for UI update
            // This ensures updateSubsequenceBar runs after the current index is set
            processorRef.queueCommand(ProcessorCommand(
                []() {}, // No-op command (index already set by triggerStepMidi)
                [this]() {
                    updateSubsequenceBar();
                }
            ));
            DBG("Step clicked: " + juce::String(stepIndex));
        };

        // Add sequencer controls component
        addAndMakeVisible(sequencerControls);

        // Wire up MIDI learn registry for sequencer controls
        sequencerControls.setRegistry(&processorRef.getMidiLearnRegistry());

        // Register UI update callbacks for MIDI control
        setupMidiUICallbacks();

        // Setup callback for control button clicks
        // Button indices: 0=Roll, 1=Page<-, 2=Page->, 3=Length+, 4=Length-, 5=Nudge<-, 6=Nudge->, 7=Shift
        sequencerControls.onButtonClicked = [this](int buttonIndex) {
            bool isShifted = sequencerControls.isShiftActive();
            auto& state = processorRef.getState();

            switch (buttonIndex) {
                case 0:  // Roll - momentary: enable random mode while held (no UI update needed)
                    state.setIsRandom(true);
                    DBG("Roll pressed - random mode ON");
                    break;
                case 1:  // Page <- OR Duplicate (when shifted)
                    if (isShifted) {
                        // Duplicate: copy entire sequence content, doubling length (capped at 64)
                        int currentLength = state.getLength();
                        int newLength = juce::jmin(currentLength * 2, ParamConstants::MAX_STEPS);
                        int stepsToCopy = newLength - currentLength;

                        // Copy steps from beginning to after current end
                        for (int i = 0; i < stepsToCopy; i++) {
                            int srcIdx = i;
                            int dstIdx = currentLength + i;
                            state.setStepNotesLength(dstIdx, state.getStepNotesLength(srcIdx));
                            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                                state.setStepNote(dstIdx, n, state.getStepNote(srcIdx, n));
                            }
                        }
                        // Update sequence length
                        state.setLength(newLength);
                        DBG("Duplicated sequence: " + juce::String(currentLength) + " -> " + juce::String(newLength) + " steps");
                    } else {
                        // Normal: Page left
                        int currentWindow = sequencerSteps.getWindowStart();
                        int newWindow = currentWindow - 8;
                        setSequencerWindow(newWindow);
                        DBG("Page left, window now at: " + juce::String(sequencerSteps.getWindowStart()));
                    }
                    break;
                case 2:  // Page -> OR Clear (when shifted)
                    if (isShifted) {
                        // Clear: set all steps in current window to rest
                        int windowStart = sequencerSteps.getWindowStart();
                        for (int i = 0; i < 8; i++) {
                            int stepIdx = windowStart + i;
                            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                                state.setStepNote(stepIdx, n, -1);
                            }
                        }
                        DBG("Cleared window starting at " + juce::String(windowStart));
                    } else {
                        // Normal: Page right
                        int currentWindow = sequencerSteps.getWindowStart();
                        int newWindow = currentWindow + 8;
                        setSequencerWindow(newWindow);
                        DBG("Page right, window now at: " + juce::String(sequencerSteps.getWindowStart()));
                    }
                    break;
                case 3:  // Length + OR Clear Step (when shifted)
                    if (isShifted) {
                        // Clear Step: set current step to rest
                        int selectedStep = state.getSelectedStepForEditing();
                        for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                            state.setStepNote(selectedStep, n, -1);
                        }
                        DBG("Cleared step " + juce::String(selectedStep));
                    } else {
                        // Normal: Length +
                        state.increaseLength();
                    }
                    break;
                case 4:  // Length - OR Step Randomize (when shifted)
                    if (isShifted) {
                        // Step: randomize current step's note
                        int selectedStep = state.getSelectedStepForEditing();
                        state.randomizeStepNote(selectedStep);
                        DBG("Randomized step " + juce::String(selectedStep));
                    } else {
                        // Normal: Length -
                        state.decreaseLength();
                        DBG("Length decreased to: " + juce::String(state.getLength()));
                    }
                    break;
                case 5:  // Nudge <- OR All Randomize (when shifted)
                    if (isShifted) {
                        // All: randomize all steps everywhere
                        for (int i = 0; i < ParamConstants::MAX_STEPS; i++) {
                            state.randomizeStepNote(i);
                        }
                        DBG("Randomized all steps");
                    } else {
                        // Normal: Nudge left
                        state.nudgeLeft();
                        DBG("Nudge left");
                    }
                    break;
                case 6:  // Nudge -> OR Shuffle (when shifted)
                    if (isShifted) {
                        // Shuffle: randomly rearrange all steps in the sequence
                        int len = state.getLength();
                        // Fisher-Yates shuffle on the step notes
                        static std::random_device rd;
                        static std::mt19937 gen(rd());
                        for (int i = len - 1; i > 0; i--) {
                            std::uniform_int_distribution<> dis(0, i);
                            int j = dis(gen);
                            // Swap steps i and j by swapping their note data
                            // Save step i data
                            int lenI = state.getStepNotesLength(i);
                            std::array<int, ParamConstants::MAX_SUBSTEPS> notesI;
                            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                                notesI[n] = state.getStepNote(i, n);
                            }
                            // Copy j to i
                            state.setStepNotesLength(i, state.getStepNotesLength(j));
                            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                                state.setStepNote(i, n, state.getStepNote(j, n));
                            }
                            // Copy saved i to j
                            state.setStepNotesLength(j, lenI);
                            for (int n = 0; n < ParamConstants::MAX_SUBSTEPS; n++) {
                                state.setStepNote(j, n, notesI[n]);
                            }
                        }
                        DBG("Shuffled all steps");
                    } else {
                        // Normal: Nudge right
                        state.nudgeRight();
                        DBG("Nudge right");
                    }
                    break;
                case 7:  // Shift - toggle shift mode (UI updates via APVTS listener)
                    state.setIsShift(!state.getIsShift());
                    DBG("Shift mode: " + juce::String(state.getIsShift() ? "ON" : "OFF"));
                    break;
                default:
                    break;
            }
        };

        // Setup callback for control button releases (for momentary buttons like Roll)
        sequencerControls.onButtonReleased = [this](int buttonIndex) {
            switch (buttonIndex) {
                case 0:  // Roll - disable random mode when released
                    processorRef.getState().setIsRandom(false);
                    DBG("Roll released - random mode OFF");
                    break;
                default:
                    break;
            }
        };
    }

    void setupNoteRangeCallbacks() {
        // Add note range controls component (right of sequencer)
        addAndMakeVisible(noteRangeControls);

        // Initialize note range controls with current scale range
        noteRangeControls.setRange(
            processorRef.getState().getScaleRangeMin(),
            processorRef.getState().getScaleRangeMax());

        // Wire up note range callback to update backend Scale
        noteRangeControls.onRangeChanged = [this](int minNote, int maxNote) {
            // Update state directly (thread-safe via APVTS)
            processorRef.getState().setScaleRangeMin(minNote);
            processorRef.getState().setScaleRangeMax(maxNote);
            DBG("Note range changed: " << minNote << " - " << maxNote);
        };
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
        if (sequencerSteps.getWindowStart() != clampedWindow) {
            sequencerSteps.setWindowStart(clampedWindow);

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
        auto& callbacks = processorRef.getMidiUICallbacks();

        // Note: Roll and Shift button UI updates are now handled automatically via APVTS listeners
        // in SequencerControlsComponent::onStateChanged()

        // Other sequencer control buttons - trigger visual update (momentary actions)
        callbacks.registerCallback(MidiControlId::BUTTON_PAGE_LEFT_SHIFT_DUPLICATE_SEQUENCE, [this](float value) {
            sequencerSteps.repaint();
            sequenceOverview.getSequenceGrid().updateFromState(processorRef.getState());
        });

        callbacks.registerCallback(MidiControlId::BUTTON_PAGE_RIGHT_SHIFT_CLEAR_WINDOW, [this](float value) {
            sequencerSteps.repaint();
        });

        callbacks.registerCallback(MidiControlId::BUTTON_LENGTH_PLUS_SHIFT_CLEAR_STEP, [this](float value) {
            sequencerSteps.repaint();
            sequenceOverview.getSequenceGrid().updateFromState(processorRef.getState());
        });

        callbacks.registerCallback(MidiControlId::BUTTON_LENGTH_MINUS_SHIFT_RANDOMIZE_STEP, [this](float value) {
            sequencerSteps.repaint();
            sequenceOverview.getSequenceGrid().updateFromState(processorRef.getState());
        });

        callbacks.registerCallback(MidiControlId::BUTTON_NUDGE_LEFT_SHIFT_RANDOMIZE_ALL, [this](float value) {
            sequencerSteps.repaint();
        });

        callbacks.registerCallback(MidiControlId::BUTTON_NUDGE_RIGHT_SHIFT_SHUFFLE, [this](float value) {
            sequencerSteps.repaint();
        });
    }

    PluginProcessor& processorRef;

    NavBarComponent navBar;
    PianoRollComponent pianoRoll;
    MidiDataPerStepSelectionComponent midiDataPerStep;
    SequencerStepsComponent sequencerSteps;
    SequencerControlsComponent sequencerControls;
    MainControlsComponent mainControls;
    SequenceOverviewComponent sequenceOverview;
    SubsequenceControlsComponent subsequenceControls;
    NoteRangeControlsComponent noteRangeControls;
};


