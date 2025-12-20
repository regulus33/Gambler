#include "PluginEditor.h"
#include "backend/ClockMode.h"

namespace PluginEditorLayout {
    // Background
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;
    // Editor size
    constexpr int editorWidth = 1012;
    constexpr int editorHeight = 560;
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
    constexpr int mainControlsOverflowBottom = 0; // Not sure why we wanted this to overflow, it means the bottom of MainControls sits under sequencer...
    constexpr int mainControlsGapFromPianoRoll = 18;
    constexpr int mainControlsWidth = 198;  // Same width as child components (no horizontal padding)
    // Sequence overview
    constexpr int sequenceOverviewGapFromMainControls = 18;
    constexpr int sequenceOverviewWidth = 198;  // Same width as MainControlsComponent
    constexpr int sequenceOverviewHeight = 260; // Title + grid + buttons
    // Subsequence controls
    constexpr int subsequenceControlsGap = 40;
    constexpr int subsequenceControlsWidth = SubsequenceControlsConstants::containerWidth;  // ~62px
    constexpr int subsequenceControlsHeight = 260;  // Same height as SequenceOverviewComponent
    // Sequencer
    constexpr int sequencerX = 22;
    constexpr int sequencerStepsY = 312;
    constexpr int sequencerWidth = 862;
    constexpr int sequencerStepsHeight = 92;
    constexpr int sequencerControlsY = 422;
    constexpr int sequencerControlsHeight = 92;
    // Note range controls (right of sequencer)
    constexpr int noteRangeControlsGap = 18;
    constexpr int noteRangeControlsRightMargin = 22;  // Matches left margin (sequencerX)
    // Inspector button
    constexpr int inspectorButtonWidth = 100;
    constexpr int inspectorButtonHeight = 25;
    constexpr int inspectorButtonMarginRight = 110;
    constexpr int inspectorButtonMarginBottom = 35;
    // Settings button (top right corner)
    constexpr int settingsButtonSize = SettingsButtonConstants::totalSize;  // 36px
    constexpr int settingsButtonOffsetRight = 46;  // Offset from right edge (positive = move left)
}

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      sequencerSteps (p.getSequence()) // Initialize with reference to processor's sequence
{
    juce::ignoreUnused (processorRef);

    // Add view toggle button
    addAndMakeVisible (viewToggleButton);

    // Setup callback for view mode changes
    viewToggleButton.onToggle = [this] (bool showPianoRoll) {
        pianoRoll.setVisible(showPianoRoll);
        midiDataPerStep.setVisible(!showPianoRoll);
        DBG ("View mode changed: " + juce::String(showPianoRoll ? "PianoRoll" : "MIDI Data"));
    };

    // Add piano roll component (visible by default)
    addAndMakeVisible (pianoRoll);

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
        triggerAsyncUpdate();
    };

    // Add MIDI data per step component (hidden by default)
    addChildComponent (midiDataPerStep);

    // Add main controls component (note/octave + probability/grid)
    addAndMakeVisible (mainControls);

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
    addAndMakeVisible (sequenceOverview);

    // Setup callbacks for sequence overview buttons
    sequenceOverview.onExportClicked = []() {
        DBG ("Export button clicked");
    };
    sequenceOverview.onSaveClicked = []() {
        DBG ("Save button clicked");
    };
    sequenceOverview.onLoadClicked = []() {
        DBG ("Load button clicked");
    };

    // Add subsequence controls component
    addAndMakeVisible (subsequenceControls);

    // Setup callbacks for subsequence controls
    subsequenceControls.onBarClicked = [this] (int index) {
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
                    DBG ("Substep " + juce::String(index) + " selected");
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
                DBG ("Subsequence length decreased to: " + juce::String(stepNotes.getLength()));
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
                DBG ("Subsequence length increased to: " + juce::String(stepNotes.getLength()));
            },
            // Completion callback: runs on message thread after command executes
            [this]() {
                updateSubsequenceBar();
            }
        ));
    };

    // Add sequencer steps component
    addAndMakeVisible (sequencerSteps);

    // Setup callback for step clicks - set current index and trigger MIDI
    sequencerSteps.onStepClicked = [this] (int stepIndex) {
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
        DBG ("Step clicked: " + juce::String (stepIndex));
    };

    // Add sequencer controls component
    addAndMakeVisible (sequencerControls);

    // Setup callback for control button clicks
    // Button indices: 0=Roll, 1=Index<-, 2=Index->, 3=Length+, 4=Length-, 5=Nudge<-, 6=Nudge->, 7=Shift
    sequencerControls.onButtonClicked = [this] (int buttonIndex) {
        switch (buttonIndex) {
            case 0:  // Roll - momentary: enable random mode while held (no UI update needed)
                processorRef.queueCommand(ProcessorCommand([this]() {
                    processorRef.getSequence().setIsRandom(true);
                    DBG("Roll pressed - random mode ON");
                }));
                break;
            case 1:  // Index <-
                processorRef.queueCommand(ProcessorCommand(
                    [this]() {
                        processorRef.getSequence().indexLeft();
                        DBG("Index left, now at: " + juce::String(processorRef.getSequence().getCurrentIndex()));
                    },
                    [this]() {
                        sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());
                        updateSubsequenceBar();
                    }
                ));
                break;
            case 2:  // Index ->
                processorRef.queueCommand(ProcessorCommand(
                    [this]() {
                        processorRef.getSequence().indexRight();
                        DBG("Index right, now at: " + juce::String(processorRef.getSequence().getCurrentIndex()));
                    },
                    [this]() {
                        sequenceOverview.getSequenceGrid().updateFromSequence(processorRef.getSequence());
                        updateSubsequenceBar();
                    }
                ));
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
    sequencerControls.onButtonReleased = [this] (int buttonIndex) {
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

    // Add note range controls component (right of sequencer)
    addAndMakeVisible (noteRangeControls);

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

    // Add logo component (above piano roll)
    addAndMakeVisible (logo);

    // Add version label (top right, left of settings button)
    addAndMakeVisible (versionLabel);
    versionLabel.setText ("v" VERSION, juce::dontSendNotification);
    auto interBoldFont = juce::Typeface::createSystemTypefaceFor(
        BinaryData::Inter_28ptBold_ttf,
        BinaryData::Inter_28ptBold_ttfSize);
    versionLabel.setFont (juce::FontOptions (interBoldFont).withHeight (11.0f));
    versionLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));
    versionLabel.setJustificationType (juce::Justification::centredRight);

    // Add settings button (top right corner)
    addAndMakeVisible (settingsButton);
    settingsButton.onClick = [this]() {
        if (!settingsWindow) {
            settingsWindow = std::make_unique<SettingsWindow>();
            settingsWindow->onClose = [this]() { settingsWindow.reset(); };

            // Wire up settings callbacks to processor's clock
            if (auto* content = dynamic_cast<SettingsWindowComponent*>(settingsWindow->getContentComponent())) {
                // Sync initial state from processor
                content->setInternalClockEnabled(processorRef.getClock().getMode() == ClockMode::INTERNAL);
                content->setTempo(processorRef.getClock().getBpm());

                content->onInternalClockChanged = [this](bool enabled) {
                    processorRef.queueCommand(ProcessorCommand([this, enabled]() {
                        if (enabled) {
                            processorRef.getClock().setMode(ClockMode::INTERNAL);
                            processorRef.getClock().start();
                        } else {
                            processorRef.getClock().setMode(ClockMode::DAW_SYNC);
                            processorRef.getClock().stop();
                        }
                        DBG("Internal clock " + juce::String(enabled ? "enabled" : "disabled"));
                    }));
                };

                content->onTempoChanged = [this](double bpm) {
                    processorRef.queueCommand(ProcessorCommand([this, bpm]() {
                        processorRef.getClock().setBpm(bpm);
                        DBG("Tempo changed to: " + juce::String(bpm) + " BPM");
                    }));
                };

                content->onPlayClicked = [this]() {
                    processorRef.queueCommand(ProcessorCommand([this]() {
                        // Reset sequence to beginning and start clock
                        processorRef.getSequence().reset();
                        processorRef.getClock().start();
                        DBG("Play clicked - sequence reset to index 0, clock started");
                    }));
                };

                content->onStopClicked = [this]() {
                    processorRef.queueCommand(ProcessorCommand([this]() {
                        processorRef.getClock().stop();
                        DBG("Stop clicked - clock stopped");
                    }));
                };

                // Sync preview notes setting from processor
                content->setPreviewNotesEnabled(processorRef.getUserPreferences().previewNotesOnEdit);
                content->onPreviewNotesChanged = [this](bool enabled) {
                    processorRef.getUserPreferences().previewNotesOnEdit = enabled;
                    DBG("Preview notes on edit: " + juce::String(enabled ? "enabled" : "disabled"));
                };
            }
        }
        settingsWindow->positionRelativeTo(this);
        settingsWindow->setVisible(true);
    };

    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector) {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // Register as keyboard listener to capture hotkeys
    // TODO: Remove keyboard listener after dev phase (currently used for XML snapshot hotkey)
    addKeyListener (this);
    setWantsKeyboardFocus (true);

    // Wire up the processor's tick callback to trigger async UI updates
    processorRef.onSequenceTick = [this]() {
        triggerAsyncUpdate();
    };

    // Wire up the completion callbacks notification to trigger async UI updates
    // This ensures UI updates happen after queued commands have executed
    processorRef.onCompletionCallbacksPending = [this]() {
        triggerAsyncUpdate();
    };

    setSize (PluginEditorLayout::editorWidth, PluginEditorLayout::editorHeight);
}

PluginEditor::~PluginEditor() {
    // Clear the callbacks before destruction to avoid dangling reference
    processorRef.onSequenceTick = nullptr;
    processorRef.onCompletionCallbacksPending = nullptr;
    cancelPendingUpdate();
    removeKeyListener (this);
}

void PluginEditor::handleAsyncUpdate() {
    // Execute any pending completion callbacks from queued commands
    // These callbacks contain UI update logic that should run after domain state changes
    int numCallbacks = processorRef.executeCompletionCallbacks();
    if (numCallbacks > 0) {
        DBG("Executed " << numCallbacks << " completion callbacks");
    }

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
}

void PluginEditor::paint (juce::Graphics& g) {
    g.fillAll (juce::Colour (PluginEditorLayout::bgColourHex));
}

void PluginEditor::resized() {
    using namespace PluginEditorLayout;

    pianoRoll.setBounds (pianoRollX, pianoRollY, pianoRollWidth, pianoRollHeight);

    // Position MIDI data per step component at same location as piano roll
    midiDataPerStep.setBounds (pianoRollX, pianoRollY, pianoRollWidth, pianoRollHeight);

    // Position view toggle button above piano roll, aligned to left edge
    int buttonX = pianoRollX;
    int buttonY = pianoRollY - buttonHeight - buttonGapAbovePianoRoll;
    viewToggleButton.setBounds (buttonX, buttonY, buttonWidth, buttonHeight);

    // Position logo centered above piano roll, 6px gap
    constexpr int logoGapAbovePianoRoll = 6;
    int logoWidth = logo.getCalculatedWidth();
    int logoHeight = logo.getCalculatedHeight();
    int logoX = pianoRollX + (pianoRollWidth - logoWidth) / 2;
    int logoY = pianoRollY - logoGapAbovePianoRoll - logoHeight;
    logo.setBounds (logoX, logoY, logoWidth, logoHeight);

    // Position main controls to the right of the piano roll / MIDI data panel
    int mainControlsX = pianoRollX + pianoRollWidth + mainControlsGapFromPianoRoll;
    int mainControlsY = pianoRollY - mainControlsOverflowTop;
    int mainControlsHeight = pianoRollHeight + mainControlsOverflowTop + mainControlsOverflowBottom;
    mainControls.setBounds (mainControlsX, mainControlsY, mainControlsWidth, mainControlsHeight);

    // Position sequence overview to the right of main controls
    int sequenceOverviewX = mainControlsX + mainControlsWidth + sequenceOverviewGapFromMainControls;
    int sequenceOverviewY = mainControlsY;
    sequenceOverview.setBounds (sequenceOverviewX, sequenceOverviewY, sequenceOverviewWidth, sequenceOverviewHeight);

    // Position subsequence controls to the right of sequence overview
    int subsequenceControlsX = sequenceOverviewX + sequenceOverviewWidth + subsequenceControlsGap;
    int subsequenceControlsY = mainControlsY;
    subsequenceControls.setBounds (subsequenceControlsX, subsequenceControlsY, subsequenceControlsWidth, subsequenceControlsHeight);

    // Position sequencer steps
    sequencerSteps.setBounds (sequencerX, sequencerStepsY, sequencerWidth, sequencerStepsHeight);

    // Position sequencer controls
    sequencerControls.setBounds (sequencerX, sequencerControlsY, sequencerWidth, sequencerControlsHeight);

    // Position note range controls (right of sequencer)
    int noteRangeControlsX = sequencerX + sequencerWidth + noteRangeControlsGap;
    int noteRangeControlsY = sequencerStepsY;
    int noteRangeControlsWidth = editorWidth - noteRangeControlsX - noteRangeControlsRightMargin;
    int noteRangeControlsHeight = (sequencerControlsY - sequencerStepsY) + sequencerControlsHeight;  // 92 + 18 gap + 92 = 202
    noteRangeControls.setBounds (noteRangeControlsX, noteRangeControlsY, noteRangeControlsWidth, noteRangeControlsHeight);

    // Version label (top right, left of settings button)
    constexpr int versionLabelWidth = 60;
    constexpr int versionLabelHeight = 20;
    constexpr int versionLabelMarginRight = 10;  // Gap between version and settings button
    int versionLabelX = getWidth() - settingsButtonSize - settingsButtonOffsetRight - versionLabelWidth - versionLabelMarginRight;
    int versionLabelY = (settingsButtonSize - versionLabelHeight) / 2;  // Vertically center with settings button
    versionLabel.setBounds (versionLabelX, versionLabelY, versionLabelWidth, versionLabelHeight);

    // Settings button in top right corner
    settingsButton.setBounds (getWidth() - settingsButtonSize - settingsButtonOffsetRight, 0,
                              settingsButtonSize, settingsButtonSize);

    // Inspector button in bottom right corner
    inspectButton.setBounds (getWidth() - inspectorButtonMarginRight,
                             getHeight() - inspectorButtonMarginBottom,
                             inspectorButtonWidth, inspectorButtonHeight);
}

void PluginEditor::setActiveStepAndNote (int stepIndex) {
    // Update the active step in the sequencer steps component
    sequencerSteps.setActiveStep (stepIndex);

    if (stepIndex >= 0 && stepIndex < 64) {
        // Get the note from this step in the sequence
        const auto& step = processorRef.getSequence().getStep (stepIndex);
        const auto& stepNotes = step.getStepNotes();
        const auto& currentNote = stepNotes.getCurrentNote();
        int midiNote = currentNote.getNoteNumber();

        // Calculate chromatic note (0-11) for piano roll
        int chromaticNote = midiNote % 12;

        // Update the piano roll active note
        pianoRoll.setActiveNote (chromaticNote);
    } else {
        // Clear both if stepIndex is -1
        pianoRoll.setActiveNote (-1);
    }
}

void PluginEditor::updateSubsequenceBar() {
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

bool PluginEditor::keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) {
    juce::ignoreUnused (originatingComponent);

    // TODO: Remove these dev hotkeys after moving on from dev phase

    // Hotkey 'x' - Save XML snapshot of Sequencer state to state.xml
    if (key.getKeyCode() == 'x' || key.getKeyCode() == 'X') {
        // Get XML from Sequence
        auto xml = processorRef.getSequence().toXml();

// Save to repository root directory as state.xml
// Use compile-time definition from CMakeLists.txt
#ifdef REPO_ROOT_PATH
        auto repoRoot = juce::File (REPO_ROOT_PATH);
#else
        // Fallback to current working directory if REPO_ROOT_PATH not defined
        auto repoRoot = juce::File::getCurrentWorkingDirectory();
#endif

        auto file = repoRoot.getChildFile ("state.xml");

        // Write XML to file
        if (xml->writeTo (file)) {
            DBG ("XML snapshot saved to: " + file.getFullPathName());

            // Optional: Show a brief message to user (you could add a label for this later)
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                "Snapshot Saved",
                "XML snapshot saved to:\n" + file.getFullPathName(),
                "OK");
        } else {
            DBG ("Failed to save XML snapshot");
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Failed to save XML snapshot to:\n" + file.getFullPathName(),
                "OK");
        }

        return true; // Key was handled
    }

    // Hotkey 'n' - Cycle through notes to test active note indicator
    if (key.getKeyCode() == 'n' || key.getKeyCode() == 'N') {
        // Cycle through chromatic scale: C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B
        int currentNote = pianoRoll.getActiveNote();
        int nextNote = (currentNote + 1) % 12;
        pianoRoll.setActiveNote (nextNote);

        // Note names for debug output
        const char* noteNames[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
        DBG ("Active note set to: " + juce::String (noteNames[nextNote]) + " (index " + juce::String (nextNote) + ")");

        return true; // Key was handled
    }

    // Hotkey 'm' - Clear active note (set to -1)
    if (key.getKeyCode() == 'm' || key.getKeyCode() == 'M') {
        pianoRoll.setActiveNote (-1);
        DBG ("Active note cleared");

        return true; // Key was handled
    }

    // Hotkey 's' - Cycle through steps to test active step indicator
    if (key.getKeyCode() == 's' || key.getKeyCode() == 'S') {
        // Cycle through first 8 steps (visible window)
        int currentStep = sequencerSteps.getActiveStep();
        int nextStep = (currentStep + 1) % 8;
        setActiveStepAndNote (nextStep);

        DBG ("Active step set to: " + juce::String (nextStep));

        return true; // Key was handled
    }

    // Hotkey 'c' - Clear active step (set to -1)
    if (key.getKeyCode() == 'c' || key.getKeyCode() == 'C') {
        setActiveStepAndNote (-1);
        DBG ("Active step cleared");

        return true; // Key was handled
    }

    return false; // Key was not handled
}
