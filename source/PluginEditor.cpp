#include "PluginEditor.h"

namespace PluginEditorLayout {
    // Background
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;
    // Editor size
    constexpr int editorWidth = 1022;
    constexpr int editorHeight = 560;
    // Piano roll / MIDI data panel
    constexpr int pianoRollX = 50;
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
    constexpr int subsequenceControlsGap = 18;  // Same gap as between MainControls and SequenceOverview
    constexpr int subsequenceControlsWidth = SubsequenceControlsConstants::containerWidth;  // ~62px
    constexpr int subsequenceControlsHeight = 260;  // Same height as SequenceOverviewComponent
    // Sequencer
    constexpr int sequencerX = 50;
    constexpr int sequencerStepsY = 312;
    constexpr int sequencerWidth = 862;
    constexpr int sequencerStepsHeight = 92;
    constexpr int sequencerControlsY = 422;
    constexpr int sequencerControlsHeight = 92;
    // Inspector button
    constexpr int inspectorButtonWidth = 100;
    constexpr int inspectorButtonHeight = 25;
    constexpr int inspectorButtonMarginRight = 110;
    constexpr int inspectorButtonMarginBottom = 35;
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
    sequencerControls.onButtonClicked = [] (int buttonIndex) {
        // TODO: Handle control button clicks when we wire up the backend
        const char* buttonNames[] = {"Roll", "Index <-", "Index ->", "Length +",
                                     "Length -", "Nudge <-", "Nudge ->", "Shift"};
        DBG ("Control button clicked: " + juce::String (buttonNames[buttonIndex]));
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

    setSize (PluginEditorLayout::editorWidth, PluginEditorLayout::editorHeight);
}

PluginEditor::~PluginEditor() {
    removeKeyListener (this);
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
