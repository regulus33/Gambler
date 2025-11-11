#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      sequencerSteps (p.getSequence()) // Initialize with reference to processor's sequence
{
    juce::ignoreUnused (processorRef);

    // Add piano roll component
    addAndMakeVisible (pianoRoll);

    // Setup callback for scale changes
    pianoRoll.onNoteChanged = []() {
        // TODO: Update backend Scale object when we wire up the backend
        DBG ("Piano roll scale changed");
    };

    // Add sequencer steps component
    addAndMakeVisible (sequencerSteps);

    // Setup callback for step clicks
    sequencerSteps.onStepClicked = [] (int stepIndex) {
        // TODO: Handle step selection when we wire up the backend
        DBG ("Step clicked: " + juce::String (stepIndex));
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

    // Set editor size to match the design (approximately 980 x 560 based on the mockup)
    setSize (980, 560);
}

PluginEditor::~PluginEditor() {
    removeKeyListener (this);
}

void PluginEditor::paint (juce::Graphics& g) {
    // Background - dark grey matching the design
    g.fillAll (juce::Colour (0xFF1E1E1E));
}

void PluginEditor::resized() {
    // Position piano roll in top-left corner with some padding
    // Based on the design mockup, it's positioned at approximately (50, 95)
    pianoRoll.setBounds (50, 95, 408, 199);

    // Position sequencer steps 18px below piano roll, left-aligned
    // Y = 95 (piano roll Y) + 199 (piano roll height) + 18 (gap) = 312
    // Size: 8 steps × 84px + 7 gaps × 16px = 784px wide, 84px tall
    sequencerSteps.setBounds (50, 312, 784, 84);

    // Inspector button in bottom right corner
    inspectButton.setBounds (getWidth() - 110, getHeight() - 35, 100, 25);
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
