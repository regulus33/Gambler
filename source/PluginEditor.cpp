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

    // Setup callback for scale changes
    pianoRoll.onNoteChanged = []() {
        // TODO: Update backend Scale object when we wire up the backend
        DBG ("Piano roll scale changed");
    };

    // Add MIDI data per step component (hidden by default)
    addChildComponent (midiDataPerStep);

    // Add main controls component (note/octave + probability/grid)
    addAndMakeVisible (mainControls);

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
    subsequenceControls.onBarClicked = [] (int index) {
        DBG ("Subsequence bar clicked: " + juce::String (index));
    };
    subsequenceControls.onMinusClicked = []() {
        DBG ("Subsequence minus button clicked");
    };
    subsequenceControls.onPlusClicked = []() {
        DBG ("Subsequence plus button clicked");
    };

    // Add sequencer steps component
    addAndMakeVisible (sequencerSteps);

    // Setup callback for step clicks
    sequencerSteps.onStepClicked = [] (int stepIndex) {
        // TODO: Handle step selection when we wire up the backend
        DBG ("Step clicked: " + juce::String (stepIndex));
    };

    // Add sequencer controls component
    addAndMakeVisible (sequencerControls);

    // Setup callback for control button clicks
    // Button indices: 0=Roll, 1=Index<-, 2=Index->, 3=Length+, 4=Length-, 5=Nudge<-, 6=Nudge->, 7=Shift
    sequencerControls.onButtonClicked = [this] (int buttonIndex) {
        auto& seq = processorRef.getSequence();

        switch (buttonIndex) {
            case 0:  // Roll - toggle random mode
                seq.setIsRandom(!seq.getIsRandom());
                sequencerControls.setButtonToggled(0, seq.getIsRandom());
                DBG("Roll (random mode): " + juce::String(seq.getIsRandom() ? "ON" : "OFF"));
                break;
            case 1:  // Index <-
                seq.indexLeft();
                DBG("Index left, now at: " + juce::String(seq.getCurrentIndex()));
                break;
            case 2:  // Index ->
                seq.indexRight();
                DBG("Index right, now at: " + juce::String(seq.getCurrentIndex()));
                break;
            case 3:  // Length +
                seq.increaseLength();
                DBG("Length increased to: " + juce::String(seq.getLength()));
                break;
            case 4:  // Length -
                seq.decreaseLength();
                DBG("Length decreased to: " + juce::String(seq.getLength()));
                break;
            case 5:  // Nudge <-
                seq.nudgeLeft();
                DBG("Nudge left");
                break;
            case 6:  // Nudge ->
                seq.nudgeRight();
                DBG("Nudge right");
                break;
            case 7:  // Shift - toggle shift mode
                seq.setIsShift(!seq.getIsShift());
                sequencerControls.setButtonToggled(7, seq.getIsShift());
                DBG("Shift mode: " + juce::String(seq.getIsShift() ? "ON" : "OFF"));
                break;
            default:
                break;
        }
    };

    // Initialize Roll button state to match processor's sequence (random is ON by default now)
    sequencerControls.setButtonToggled(0, processorRef.getSequence().getIsRandom());

    // Add note range controls component (right of sequencer)
    addAndMakeVisible (noteRangeControls);

    // Add logo component (above piano roll)
    addAndMakeVisible (logo);

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
                    if (enabled) {
                        processorRef.getClock().setMode(ClockMode::INTERNAL);
                        processorRef.getClock().start();
                    } else {
                        processorRef.getClock().setMode(ClockMode::DAW_SYNC);
                        processorRef.getClock().stop();
                    }
                    DBG("Internal clock " + juce::String(enabled ? "enabled" : "disabled"));
                };

                content->onTempoChanged = [this](double bpm) {
                    processorRef.getClock().setBpm(bpm);
                    DBG("Tempo changed to: " + juce::String(bpm) + " BPM");
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

    setSize (PluginEditorLayout::editorWidth, PluginEditorLayout::editorHeight);
}

PluginEditor::~PluginEditor() {
    // Clear the callback before destruction to avoid dangling reference
    processorRef.onSequenceTick = nullptr;
    cancelPendingUpdate();
    removeKeyListener (this);
}

void PluginEditor::handleAsyncUpdate() {
    // Update the active step indicator to follow the sequence playback
    int currentStep = processorRef.getSequence().getCurrentIndex();
    setActiveStepAndNote(currentStep);

    // Repaint the sequencer steps to show updated note values
    sequencerSteps.repaint();
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
