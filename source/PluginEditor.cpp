#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    // Add piano roll component
    addAndMakeVisible(pianoRoll);

    // Setup callback for scale changes
    pianoRoll.onNoteChanged = []() {
        // TODO: Update backend Scale object when we wire up the backend
        DBG("Piano roll scale changed");
    };

    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };

    // Register as keyboard listener to capture hotkeys
    // TODO: Remove keyboard listener after dev phase (currently used for XML snapshot hotkey)
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Set editor size to match the design (approximately 980 x 560 based on the mockup)
    setSize (980, 560);
}

PluginEditor::~PluginEditor()
{
    removeKeyListener(this);
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Background - dark grey matching the design
    g.fillAll (juce::Colour(0xFF1E1E1E));
}

void PluginEditor::resized()
{
    // Position piano roll in top-left corner with some padding
    // Based on the design mockup, it's positioned at approximately (50, 95)
    pianoRoll.setBounds(50, 95, 408, 199);

    // Inspector button in bottom right corner
    inspectButton.setBounds(getWidth() - 110, getHeight() - 35, 100, 25);
}

bool PluginEditor::keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent)
{
    juce::ignoreUnused(originatingComponent);

    // TODO: Remove this dev hotkey after moving on from dev phase
    // Hotkey 'x' - Save XML snapshot of Sequencer state to state.xml
    if (key.getKeyCode() == 'x' || key.getKeyCode() == 'X')
    {
        // Get XML from Sequence
        auto xml = processorRef.getSequence().toXml();

        // Save to repository root directory as state.xml
        // Use compile-time definition from CMakeLists.txt
        #ifdef REPO_ROOT_PATH
            auto repoRoot = juce::File(REPO_ROOT_PATH);
        #else
            // Fallback to current working directory if REPO_ROOT_PATH not defined
            auto repoRoot = juce::File::getCurrentWorkingDirectory();
        #endif

        auto file = repoRoot.getChildFile("state.xml");

        // Write XML to file
        if (xml->writeTo(file))
        {
            DBG("XML snapshot saved to: " + file.getFullPathName());

            // Optional: Show a brief message to user (you could add a label for this later)
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "Snapshot Saved",
                "XML snapshot saved to:\n" + file.getFullPathName(),
                "OK"
            );
        }
        else
        {
            DBG("Failed to save XML snapshot");
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Failed to save XML snapshot to:\n" + file.getFullPathName(),
                "OK"
            );
        }

        return true; // Key was handled
    }

    return false; // Key was not handled
}
