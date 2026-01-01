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

    // Make window resizable with aspect ratio constraints
    setResizable (true, true);

    // Set minimum size (50% of design size)
    setResizeLimits (PluginEditorLayout::editorWidth / 2,
                     PluginEditorLayout::editorHeight / 2,
                     PluginEditorLayout::editorWidth * 4,
                     PluginEditorLayout::editorHeight * 4);

    // Add component listener to self for aspect ratio constraint
    addComponentListener (this);

    setSize (PluginEditorLayout::editorWidth, PluginEditorLayout::editorHeight);
}

PluginEditor::~PluginEditor() {
    removeKeyListener (this);
    removeComponentListener (this);
}

void PluginEditor::paint (juce::Graphics& g) {
    g.fillAll (juce::Colour (PluginEditorLayout::bgColourHex));
}

void PluginEditor::resized() {
    using namespace PluginEditorLayout;

    // Calculate scale factor
    float scale = getScaleFactor();

    // Set the application component to the design size
    // It will be scaled by the transform
    applicationComponent.setBounds (0, 0, editorWidth, editorHeight);

    // Apply scaling transform to the application component
    applicationComponent.setTransform (juce::AffineTransform::scale (scale));

    // Inspector button in bottom right corner (uses scaled coordinates)
    inspectButton.setBounds (editorWidth - inspectorButtonMarginRight,
                             editorHeight - inspectorButtonMarginBottom,
                             inspectorButtonWidth, inspectorButtonHeight);

    // Apply same scaling transform to inspector button
    inspectButton.setTransform (juce::AffineTransform::scale (scale));
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

    // Hotkey 'x' - Save XML snapshot of full state to state.xml
    if (key.getKeyCode() == 'x' || key.getKeyCode() == 'X') {
        // Get full state XML (includes APVTS, Clock, UserPreferences)
        auto xml = processorRef.getStateAsXml();

        // Save to repository root directory as state.xml
#ifdef REPO_ROOT_PATH
        auto repoRoot = juce::File (REPO_ROOT_PATH);
#else
        auto repoRoot = juce::File::getCurrentWorkingDirectory();
#endif

        auto file = repoRoot.getChildFile ("state.xml");

        // Write XML to file
        if (xml->writeTo (file)) {
            DBG ("XML snapshot saved to: " + file.getFullPathName());
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::InfoIcon,
                "Snapshot Saved",
                "State saved to:\n" + file.getFullPathName(),
                "OK");
        } else {
            DBG ("Failed to save XML snapshot");
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Could not save state to:\n" + file.getFullPathName(),
                "OK");
        }

        return true;
    }

    // Hotkey 'l' - Load XML snapshot from state.xml
    if (key.getKeyCode() == 'l' || key.getKeyCode() == 'L') {
#ifdef REPO_ROOT_PATH
        auto repoRoot = juce::File (REPO_ROOT_PATH);
#else
        auto repoRoot = juce::File::getCurrentWorkingDirectory();
#endif

        auto file = repoRoot.getChildFile ("state.xml");

        if (!file.existsAsFile()) {
            DBG ("state.xml not found at: " + file.getFullPathName());
            juce::AlertWindow::showMessageBoxAsync (
                juce::AlertWindow::WarningIcon,
                "Load Failed",
                "state.xml not found at:\n" + file.getFullPathName(),
                "OK");
            return true;
        }

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

        // Load state via processor method
        processorRef.loadStateFromXml(*xml);

        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::InfoIcon,
            "Snapshot Loaded",
            "State loaded from:\n" + file.getFullPathName(),
            "OK");

        return true;
    }

    return false;
}

float PluginEditor::getScaleFactor() const {
    using namespace PluginEditorLayout;

    // Calculate scale based on width (height will be constrained by aspect ratio)
    float scaleX = static_cast<float>(getWidth()) / static_cast<float>(editorWidth);
    float scaleY = static_cast<float>(getHeight()) / static_cast<float>(editorHeight);

    // Use the smaller scale to ensure everything fits
    return juce::jmin (scaleX, scaleY);
}

void PluginEditor::componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) {
    using namespace PluginEditorLayout;

    // Ignore if not resized or if we're already adjusting
    if (!wasResized || isAdjustingSize || &component != this) {
        return;
    }

    // Calculate aspect ratio
    const float aspectRatio = static_cast<float>(editorWidth) / static_cast<float>(editorHeight);

    // Get current size
    int currentWidth = getWidth();
    int currentHeight = getHeight();

    // Calculate what the height should be for the current width
    int targetHeight = juce::roundToInt (static_cast<float>(currentWidth) / aspectRatio);

    // If height doesn't match aspect ratio, adjust it
    if (currentHeight != targetHeight) {
        isAdjustingSize = true;
        setSize (currentWidth, targetHeight);
        isAdjustingSize = false;
    }
}