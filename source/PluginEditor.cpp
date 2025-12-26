#include "PluginEditor.h"
#include "./utility/ComponentSnapshotUtility.h"

namespace PluginEditorLayout {
    // Background
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;
    // Editor size
    constexpr int editorWidth = 1012;
    constexpr int editorHeight = 560;
    // Inspector button
    constexpr int inspectorButtonWidth = 100;
    constexpr int inspectorButtonHeight = 25;
    constexpr int inspectorButtonMarginRight = 110;
    constexpr int inspectorButtonMarginBottom = 35;
}

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      applicationComponent (p)
{
    juce::ignoreUnused (processorRef);

    // Add application component
    addAndMakeVisible (applicationComponent);

    // Add inspector button
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

    // Application component takes up the full area
    applicationComponent.setBounds (getLocalBounds());

    // Inspector button in bottom right corner
    inspectButton.setBounds (getWidth() - inspectorButtonMarginRight,
                             getHeight() - inspectorButtonMarginBottom,
                             inspectorButtonWidth, inspectorButtonHeight);
}

void PluginEditor::snapshotAllComponents(juce::Component* root, const juce::String& basePath, int& count) {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1
    if (!root || root->getWidth() <= 0 || root->getHeight() <= 0) {
        return;
    }

    // Get component type name
    juce::String componentType = juce::String(typeid(*root).name());

    // Clean up the typename
    componentType = componentType.fromLastOccurrenceOf("::", false, false);  // Remove namespace
    componentType = componentType.replaceCharacter(' ', '_');
    componentType = componentType.replaceCharacter('<', '_');
    componentType = componentType.replaceCharacter('>', '_');

    // width and height
    auto width = root->getWidth();
    auto height = root->getHeight();
    juce::String widthHeightString = "__" + juce::String(width) + "X" + juce::String(height);

    // Create filename with count for uniqueness
    juce::String filename = juce::String(count).paddedLeft('0', 3) + "_" + componentType + widthHeightString;

    // Take the snapshot
    ComponentSnapshotUtility::saveSnapshot(*root, basePath, filename, 2.0f);

    count++;

    // Recursively snapshot all children
    for (int i = 0; i < root->getNumChildComponents(); ++i) {
        snapshotAllComponents(root->getChildComponent(i), basePath, count);
    }
#else
    juce::ignoreUnused(root, basePath, count);
#endif
}

bool PluginEditor::keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) {
    juce::ignoreUnused (originatingComponent);

    // TODO: Remove these dev hotkeys after moving on from dev phase

    // Hotkey 's' - Snapshot all components recursively
    if (key.getKeyCode() == 's' || key.getKeyCode() == 'S') {
#if defined(TAKE_SNAPSHOTS_OF_COMPONENTS) && TAKE_SNAPSHOTS_OF_COMPONENTS == 1 && defined(SNAPSHOT_DESTINATION)
        // Use compile-time snapshot destination
        auto snapshotsDir = juce::File(SNAPSHOT_DESTINATION);
        snapshotsDir.createDirectory();

        juce::String basePath = snapshotsDir.getFullPathName();

        // Start recursive snapshot
        int count = 0;
        snapshotAllComponents(this, basePath, count);

        DBG("Saved " + juce::String(count) + " component snapshots to: " + basePath);

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "Snapshots Saved",
            "Saved " + juce::String(count) + " component snapshots to:\n" + basePath,
            "OK");
#else
        DBG("Component snapshots disabled - set TAKE_SNAPSHOTS_OF_COMPONENTS=1 and define SNAPSHOT_DESTINATION");
#endif

        return true;
    }

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
                "Sequence state saved to:\n" + file.getFullPathName(),
                "OK");
        } else {
            DBG ("Failed to save XML snapshot");
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Could not save sequence state to:\n" + file.getFullPathName(),
                "OK");
        }

        return true;
    }

    // Hotkey 'l' - Load XML snapshot from state.xml
    if (key.getKeyCode() == 'l' || key.getKeyCode() == 'L') {
        // Get file path
#ifdef REPO_ROOT_PATH
        auto repoRoot = juce::File (REPO_ROOT_PATH);
#else
        auto repoRoot = juce::File::getCurrentWorkingDirectory();
#endif

        auto file = repoRoot.getChildFile ("state.xml");

        // Check if file exists
        if (!file.existsAsFile()) {
            DBG ("state.xml not found at: " + file.getFullPathName());
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "Load Failed",
                "state.xml not found at:\n" + file.getFullPathName(),
                "OK");
            return true;
        }

        // Parse XML from file
        auto xml = juce::parseXML (file);
        if (!xml) {
            DBG ("Failed to parse XML from: " + file.getFullPathName());
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "Load Failed",
                "Could not parse XML from:\n" + file.getFullPathName(),
                "OK");
            return true;
        }

        // Queue command to load state on audio thread
        // Convert unique_ptr to shared_ptr for lambda capture
        auto xmlShared = std::shared_ptr<juce::XmlElement>(xml.release());
        processorRef.queueCommand(ProcessorCommand(
            [this, xmlShared]() {
                processorRef.getSequence().fromXml(*xmlShared);
                DBG("Sequence state loaded from XML");
            },
            [this]() {
                // Trigger async update to refresh all UI components
                applicationComponent.getMainScreen().updateFromSequence();
            }
        ));

        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::InfoIcon,
            "Snapshot Loaded",
            "Sequence state loaded from:\n" + file.getFullPathName(),
            "OK");

        return true;
    }

    return false;
}