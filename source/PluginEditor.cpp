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



bool PluginEditor::keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) {
    juce::ignoreUnused (originatingComponent);

    // Hotkey 's' - Snapshot all components recursively
    if (key.getKeyCode() == 's' || key.getKeyCode() == 'S') {
        ComponentSnapshotUtility::snapshotAllComponentsToDestination(this);
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