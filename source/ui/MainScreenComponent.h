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
#include "ViewToggleButton.h"
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
    // View toggle button
    constexpr int buttonWidth = 46;
    constexpr int buttonHeight = 30;
    constexpr int buttonGapAbovePianoRoll = 10;
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
          sequencerSteps(processor.getSequence()),
          midiDataPerStep(processor),
          mainControls(processor)
    {
        // Add navbar
        addAndMakeVisible(navBar);

        // Add view toggle button
        addAndMakeVisible(viewToggleButton);

        // Setup callback for view mode changes
        viewToggleButton.onToggle = [this](bool showPianoRoll) {
            pianoRoll.setVisible(showPianoRoll);
            midiDataPerStep.setVisible(!showPianoRoll);
            DBG("View mode changed: " + juce::String(showPianoRoll ? "PianoRoll" : "MIDI Data"));
        };

        // Add piano roll component (visible by default)
        addAndMakeVisible(pianoRoll);

        // Initialize piano roll with the current scale from the sequence
        pianoRoll.setScale(processorRef.getSequence().getScale());

        // Setup callback for scale changes - update backend Scale when user toggles notes
        pianoRoll.onNoteChanged = [this]() {
            // Get the scale from the piano roll
            Scale newScale = pianoRoll.getScale();

            // Queue command to update the sequence's scale and quantize all notes
            processorRef.queueCommand(ProcessorCommand([this, newScale]() {
                auto& seq = processorRef.getSequence();
                seq.getScale().setScaleNotes(newScale.getScaleNotes());
                // Quantize all existing notes to the new scale
                seq.quantizeAllNotesToScale();
                DBG("Piano roll scale changed - updated backend Scale and quantized all notes");
            }));

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

        // Wire up note controls callbacks to update the current step's note
        mainControls.getNoteControls().onNoteChanged = [this](int pitchClass) {
            // Calculate the new note for MIDI preview (before command executes)
            auto& seq = processorRef.getSequence();
            int currentStep = seq.getCurrentIndex();
            if (currentStep < 0) currentStep = 0;
            int currentNote = seq.getStep(currentStep).getStepNotes().getCurrentNote().getNoteNumber();
            int currentOctave = currentNote / 12;
            int newNote = currentOctave * 12 + pitchClass;

            // Queue command to modify state on audio thread, with completion callback for UI updates
            processorRef.queueCommand(ProcessorCommand(
                // Command: runs on audio thread
                [this, pitchClass, newNote]() {
                    auto& seq = processorRef.getSequence();
                    int currentStep = seq.getCurrentIndex();
                    auto& step = seq.getStep(currentStep);
                    step.updateCurrentNoteNumber(newNote);
                    DBG("Note changed to pitch class " << pitchClass << " -> MIDI note " << newNote);
                },
                // Completion callback: runs on message thread after command executes
                [this]() {
                    // Repaint UI after state has been updated
                    sequencerSteps.repaint();

                    // Update sequence overview grid to reflect the note color change
                    sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());

                    // Update subsequence bar to reflect the note color change
                    updateSubsequenceBar();
                }
            ));

            // Trigger MIDI preview if enabled - pass the new note directly to avoid race condition
            bool previewEnabled = processorRef.getUserPreferences().previewNotesOnEdit;
            if (previewEnabled) {
                processorRef.triggerCurrentStepMidi(newNote);
            }
        };

        mainControls.getNoteControls().onOctaveChanged = [this](int octaveIndex) {
            // Calculate the new note for MIDI preview (before command executes)
            auto& seq = processorRef.getSequence();
            int currentStep = seq.getCurrentIndex();
            if (currentStep < 0) currentStep = 0;
            int currentNote = seq.getStep(currentStep).getStepNotes().getCurrentNote().getNoteNumber();
            int pitchClass = currentNote % 12;
            int newNote = octaveIndex * 12 + pitchClass;

            // Queue command to modify state on audio thread, with completion callback for UI updates
            processorRef.queueCommand(ProcessorCommand(
                // Command: runs on audio thread
                [this, octaveIndex, newNote]() {
                    auto& seq = processorRef.getSequence();
                    int currentStep = seq.getCurrentIndex();
                    auto& step = seq.getStep(currentStep);
                    step.updateCurrentNoteNumber(newNote);
                    DBG("Octave changed to " << octaveIndex << " -> MIDI note " << newNote);
                },
                // Completion callback: runs on message thread after command executes
                [this]() {
                    // Repaint UI after state has been updated
                    sequencerSteps.repaint();

                    // Update sequence overview grid to reflect the note color change
                    sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());

                    // Update subsequence bar to reflect the note color change
                    updateSubsequenceBar();
                }
            ));

            // Trigger MIDI preview if enabled - pass the new note directly to avoid race condition
            bool previewEnabled = processorRef.getUserPreferences().previewNotesOnEdit;
            if (previewEnabled) {
                processorRef.triggerCurrentStepMidi(newNote);
            }
        };

        // Add sequence overview component
        addAndMakeVisible(sequenceOverview);

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

        // Position view toggle button above piano roll, aligned to left edge
        int buttonX = pianoRollX;
        int buttonY = pianoRollY - buttonHeight - buttonGapAbovePianoRoll;
        viewToggleButton.setBounds(buttonX, buttonY, buttonWidth, buttonHeight);

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
            // Get the note from this step in the sequence
            const auto& step = processorRef.getSequence().getStep(stepIndex);
            const auto& stepNotes = step.getStepNotes();
            const auto& currentNote = stepNotes.getCurrentNote();
            int midiNote = currentNote.getNoteNumber();

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

    // Helper to update the subsequence bar UI from current step's StepNotes
    void updateSubsequenceBar() {
        auto& seq = processorRef.getSequence();
        int currentStep = seq.getCurrentIndex();
        if (currentStep < 0) currentStep = 0;

        const auto& stepNotes = seq.getStep(currentStep).getStepNotes();
        int length = stepNotes.getLength();
        int currentSubstepIndex = stepNotes.getCurrentIndex();

        auto& bar = subsequenceControls.getSubsequenceBar();

        // Update all 8 bars based on the current step's StepNotes
        for (int i = 0; i < SubsequenceControlsConstants::numBars; ++i) {
            if (i < length) {
                // Active substep - color based on note value
                int midiNote = stepNotes.getNote(i).getNoteNumber();
                int chromaticNote = midiNote % 12;
                juce::Colour noteColor = MusicConstants::NOTE_COLOURS[chromaticNote];
                bool isCurrentSubstep = (i == currentSubstepIndex);
                bar.setBarState(i, BarState(noteColor, isCurrentSubstep));
            } else {
                // Inactive substep - show as default grey (darker)
                bar.setBarState(i, BarState(juce::Colour(SubsequenceControlsConstants::defaultGreyHex), false));
            }
        }
    }

    // Update UI from sequence state (called from async update)
    void updateFromSequence() {
        // Update the active step indicator to follow the sequence playback
        int currentStep = processorRef.getSequence().getCurrentIndex();
        setActiveStepAndNote(currentStep);

        // Repaint the sequencer steps to show updated note values
        sequencerSteps.repaint();

        // Update the note range gauge to highlight the current playing note
        const auto& step = processorRef.getSequence().getStep(currentStep);
        int currentNote = step.getStepNotes().getCurrentNote().getNoteNumber();
        noteRangeControls.setActiveNote(currentNote);

        // Update the note/octave controls to reflect the current step's note
        mainControls.getNoteControls().setFromMidiNote(currentNote);

        // Update the sequence overview grid to show all 64 steps with current step highlighted
        sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());

        // Update the subsequence bar to reflect current step's substep state
        updateSubsequenceBar();

        // Update the probability gauge to highlight the randomly selected circle (if roll is engaged)
        int lastProbabilityIndex = processorRef.getSequence().getLastProbabilityIndex();
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
            processorRef.queueCommand(ProcessorCommand(
                // Command: runs on audio thread
                [this, index]() {
                    auto& seq = processorRef.getSequence();
                    int currentStep = seq.getCurrentIndex();
                    if (currentStep < 0) currentStep = 0;

                    auto& stepNotes = seq.getStep(currentStep).getStepNotes();

                    // Only allow clicking on active substeps (within length)
                    if (index < stepNotes.getLength()) {
                        stepNotes.setCurrentIndex(index);
                        DBG("Substep " + juce::String(index) + " selected");
                    }
                },
                // Completion callback: runs on message thread after command executes
                [this]() {
                    updateSubsequenceBar();

                    auto& seq = processorRef.getSequence();
                    int currentStep = seq.getCurrentIndex();
                    if (currentStep >= 0) {
                        auto& stepNotes = seq.getStep(currentStep).getStepNotes();
                        int midiNote = stepNotes.getCurrentNote().getNoteNumber();
                        int chromaticNote = midiNote % 12;
                        pianoRoll.setActiveNote(chromaticNote);
                        mainControls.getNoteControls().setFromMidiNote(midiNote);
                    }
                }
            ));
        };
        subsequenceControls.onMinusClicked = [this]() {
            processorRef.queueCommand(ProcessorCommand(
                // Command: runs on audio thread
                [this]() {
                    auto& seq = processorRef.getSequence();
                    int currentStep = seq.getCurrentIndex();
                    if (currentStep < 0) currentStep = 0;
                    auto& stepNotes = seq.getStep(currentStep).getStepNotes();
                    stepNotes.decreaseLength();
                    DBG("Subsequence length decreased to: " + juce::String(stepNotes.getLength()));
                },
                // Completion callback: runs on message thread after command executes
                [this]() {
                    updateSubsequenceBar();
                }
            ));
        };
        subsequenceControls.onPlusClicked = [this]() {
            processorRef.queueCommand(ProcessorCommand(
                // Command: runs on audio thread
                [this]() {
                    auto& seq = processorRef.getSequence();
                    int currentStep = seq.getCurrentIndex();
                    if (currentStep < 0) currentStep = 0;
                    auto& stepNotes = seq.getStep(currentStep).getStepNotes();
                    stepNotes.increaseLength();
                    DBG("Subsequence length increased to: " + juce::String(stepNotes.getLength()));
                },
                // Completion callback: runs on message thread after command executes
                [this]() {
                    updateSubsequenceBar();
                }
            ));
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

        // Setup callback for control button clicks
        // Button indices: 0=Roll, 1=Page<-, 2=Page->, 3=Length+, 4=Length-, 5=Nudge<-, 6=Nudge->, 7=Shift
        sequencerControls.onButtonClicked = [this](int buttonIndex) {
            switch (buttonIndex) {
                case 0:  // Roll - momentary: enable random mode while held (no UI update needed)
                    processorRef.queueCommand(ProcessorCommand([this]() {
                        processorRef.getSequence().setIsRandom(true);
                        DBG("Roll pressed - random mode ON");
                    }));
                    break;
                case 1:  // Page <- (page view window left by 8 steps)
                    {
                        int currentWindow = sequencerSteps.getWindowStart();
                        int newWindow = juce::jmax(0, currentWindow - 8);
                        sequencerSteps.setWindowStart(newWindow);
                        DBG("Page left, window now at: " + juce::String(newWindow));
                    }
                    break;
                case 2:  // Page -> (page view window right by 8 steps)
                    {
                        int currentWindow = sequencerSteps.getWindowStart();
                        int newWindow = juce::jmin(56, currentWindow + 8);
                        sequencerSteps.setWindowStart(newWindow);
                        DBG("Page right, window now at: " + juce::String(newWindow));
                    }
                    break;
                case 3:  // Length +
                    processorRef.queueCommand(ProcessorCommand(
                        [this]() {
                            processorRef.getSequence().increaseLength();
                            DBG("Length increased to: " + juce::String(processorRef.getSequence().getLength()));
                        },
                        [this]() {
                            sequencerSteps.repaint();
                            sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());
                        }
                    ));
                    break;
                case 4:  // Length -
                    processorRef.queueCommand(ProcessorCommand(
                        [this]() {
                            processorRef.getSequence().decreaseLength();
                            DBG("Length decreased to: " + juce::String(processorRef.getSequence().getLength()));
                        },
                        [this]() {
                            sequencerSteps.repaint();
                            sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());
                        }
                    ));
                    break;
                case 5:  // Nudge <-
                    processorRef.queueCommand(ProcessorCommand(
                        [this]() {
                            processorRef.getSequence().nudgeLeft();
                            DBG("Nudge left");
                        },
                        [this]() {
                            sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());
                        }
                    ));
                    break;
                case 6:  // Nudge ->
                    processorRef.queueCommand(ProcessorCommand(
                        [this]() {
                            processorRef.getSequence().nudgeRight();
                            DBG("Nudge right");
                        },
                        [this]() {
                            sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());
                        }
                    ));
                    break;
                case 7:  // Shift - toggle shift mode
                    processorRef.queueCommand(ProcessorCommand(
                        [this]() {
                            auto& seq = processorRef.getSequence();
                            seq.setIsShift(!seq.getIsShift());
                            DBG("Shift mode: " + juce::String(seq.getIsShift() ? "ON" : "OFF"));
                        },
                        [this]() {
                            sequencerControls.setButtonToggled(7, processorRef.getSequence().getIsShift());
                        }
                    ));
                    break;
                default:
                    break;
            }
        };

        // Setup callback for control button releases (for momentary buttons like Roll)
        sequencerControls.onButtonReleased = [this](int buttonIndex) {
            switch (buttonIndex) {
                case 0:  // Roll - disable random mode when released
                    processorRef.queueCommand(ProcessorCommand([this]() {
                        processorRef.getSequence().setIsRandom(false);
                        DBG("Roll released - random mode OFF");
                    }));
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
            processorRef.getSequence().getScale().getRangeMin(),
            processorRef.getSequence().getScale().getRangeMax());

        // Wire up note range callback to update backend Scale
        noteRangeControls.onRangeChanged = [this](int minNote, int maxNote) {
            processorRef.queueCommand(ProcessorCommand([this, minNote, maxNote]() {
                auto& scale = processorRef.getSequence().getScale();
                scale.setRangeMin(minNote);
                scale.setRangeMax(maxNote);
                DBG("Note range changed: " << minNote << " - " << maxNote);
            }));
        };
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
    ViewToggleButton viewToggleButton;
    NoteRangeControlsComponent noteRangeControls;
};


