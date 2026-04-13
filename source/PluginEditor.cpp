#include "PluginEditor.h"
#include "./utility/ComponentSnapshotUtility.h"

namespace PluginEditorLayout {
    // Background
    constexpr juce::uint32 bgColourHex = 0xFF1E1E1E;
    // Editor size
    constexpr int editorWidth = 1012;
    constexpr int editorHeight = MainScreenComponent::fullDesignHeight;
    // Inspector button
    constexpr int inspectorButtonWidth = 100;
    constexpr int inspectorButtonHeight = 25;
    constexpr int inspectorButtonMarginRight = 110;
    constexpr int inspectorButtonMarginBottom = 35;

    // Auto-compact: aspect ratio thresholds (width/height)
    // If the window aspect ratio exceeds this, auto-enter compact bottom mode.
    // Separate constants for iOS AUv3 vs desktop VST3 — tweak independently.
    // TODO: Iterate on these values with real-device testing.
#if JUCE_IOS
    constexpr float autoCompactAspectRatio = 2.8f;
#else
    constexpr float autoCompactAspectRatio = 2.8f;
#endif
}

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      applicationComponent (p)
{
    juce::ignoreUnused (processorRef);

    // Add application component
    addAndMakeVisible (applicationComponent);

    // In standalone mode, create a virtual MIDI keyboard that captures computer
    // keyboard input. Set SHOW_MIDI_KEYBOARD=1 in CMakeLists to display the piano strip.
    if (processorRef.wrapperType == juce::AudioProcessor::wrapperType_Standalone) {
        midiKeyboard = std::make_unique<juce::MidiKeyboardComponent>(
            processorRef.getKeyboardState(),
            juce::MidiKeyboardComponent::horizontalKeyboard);
        midiKeyboard->setKeyPressBaseOctave (4);
        midiKeyboard->setWantsKeyboardFocus (true);
#if SHOW_MIDI_KEYBOARD
        addAndMakeVisible (*midiKeyboard);
#else
        midiKeyboard->setSize (0, 0);
        addChildComponent (*midiKeyboard);
#endif
    }

#if SHOW_INSPECTOR
    // Add inspector button (only when SHOW_INSPECTOR=1)
    addAndMakeVisible (inspectButton);

    // this chunk of code instantiates and opens the melatonin inspector
    inspectButton.onClick = [&] {
        if (!inspector) {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }

        inspector->setVisible (true);
    };
#endif

    // Register as keyboard listener to capture hotkeys
    addKeyListener (this);
    setWantsKeyboardFocus (true);

    // Make window resizable. On iOS the host controls sizing so skip the
    // corner drag-handle (second param) — it's non-functional and just wastes space.
#if JUCE_IOS
    setResizable (true, false);
#else
    setResizable (true, true);
#endif

    // Set minimum size (50% of design size, min height allows compact mode)
    setResizeLimits (PluginEditorLayout::editorWidth / 2,
                     150,  // Allow compact mode heights
                     PluginEditorLayout::editorWidth * 4,
                     PluginEditorLayout::editorHeight * 4);

    // Add component listener to self for aspect ratio constraint
    addComponentListener (this);

    // Wire up compact mode design height callback
    applicationComponent.onDesignHeightChanged = [this](int newDesignHeight) {
        currentDesignHeight = newDesignHeight;
        isAdjustingSize = true;
        float scale = static_cast<float>(getWidth()) / static_cast<float>(PluginEditorLayout::editorWidth);
        int newWindowHeight = juce::roundToInt(static_cast<float>(newDesignHeight) * scale) + getKeyboardStripHeight();
        setSize(getWidth(), newWindowHeight);
        isAdjustingSize = false;
    };

    setSize (PluginEditorLayout::editorWidth, PluginEditorLayout::editorHeight + getKeyboardStripHeight());
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

    // On first real layout, defer auto-compact check to run after this resized() completes.
    // This ensures the editor has its final host-provided dimensions before we trigger
    // compact mode, which matters on iOS where setSize() is a no-op.
    if (!autoCompactApplied && getWidth() > 0 && getHeight() > 0) {
        autoCompactApplied = true;
        juce::MessageManager::callAsync([this]() { checkAutoCompact(); });
    }

    // In compact mode, scale to fit the width only — ignore the height constraint
    // so the short compact content isn't shrunk to fit a tall window's aspect ratio.
    // In full mode, scale to fit both dimensions as before.
    bool isCompact = (currentDesignHeight != MainScreenComponent::fullDesignHeight);
    float scaleX = static_cast<float>(getWidth()) / static_cast<float>(editorWidth);
    float scale = isCompact ? scaleX : getScaleFactor();

    // Horizontal: center; Vertical: anchor to top — excess space falls below,
    // which is where the window grows/shrinks from.
    int scaledWidth = juce::roundToInt (editorWidth * scale);
    int offsetX = (getWidth() - scaledWidth) / 2;
    int offsetY = 0;

    // Set the application component to the design size
    // It will be scaled by the transform
    applicationComponent.setBounds (0, 0, editorWidth, currentDesignHeight);

    // Apply scaling transform with centering offset to the application component
    applicationComponent.setTransform (juce::AffineTransform::scale (scale)
                                                          .translated (static_cast<float>(offsetX),
                                                                      static_cast<float>(offsetY)));

#if SHOW_MIDI_KEYBOARD
    // Keyboard strip at the bottom of the window (no transform, raw pixels)
    if (midiKeyboard) {
        midiKeyboard->setBounds (0, getHeight() - keyboardHeight, getWidth(), keyboardHeight);
        midiKeyboard->toFront (false);
    }
#endif

#if SHOW_INSPECTOR
    // Inspector button in bottom right corner (uses scaled coordinates)
    inspectButton.setBounds (editorWidth - inspectorButtonMarginRight,
                             currentDesignHeight - inspectorButtonMarginBottom,
                             inspectorButtonWidth, inspectorButtonHeight);

    // Apply same scaling transform with centering offset to inspector button
    inspectButton.setTransform (juce::AffineTransform::scale (scale)
                                                     .translated (static_cast<float>(offsetX),
                                                                 static_cast<float>(offsetY)));
#endif
}

void PluginEditor::checkAutoCompact() {
    using namespace PluginEditorLayout;

#if JUCE_IOS
    // On iOS AUv3, hosts always show plugins in a constrained strip.
    // getWidth()/getHeight() reflects our requested size, not the host's actual viewport,
    // so aspect ratio detection doesn't work. Always start in compact mode.
    applicationComponent.getNavBar().enterCompactBottomMode();
#else
    float aspectRatio = static_cast<float>(getWidth()) / static_cast<float>(getHeight());

    if (aspectRatio >= autoCompactAspectRatio) {
        // Window is wide and short — auto-enter compact bottom mode
        applicationComponent.getNavBar().enterCompactBottomMode();
    }
#endif
}



bool PluginEditor::keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) {
    juce::ignoreUnused (originatingComponent);

    // Hotkey 's' - Snapshot all components recursively
    if (key.getKeyCode() == 's' || key.getKeyCode() == 'S') {
        ComponentSnapshotUtility::snapshotAllComponentsToDestination(this);
        return true;
    }

    // Forward to the hidden MIDI keyboard so computer keys trigger notes
    if (midiKeyboard && midiKeyboard->keyPressed (key))
        return true;

    return false;
}

bool PluginEditor::keyStateChanged (bool isKeyDown, juce::Component* originatingComponent) {
    juce::ignoreUnused (originatingComponent);

    // Forward to the hidden MIDI keyboard so it can detect key-up for note-off
    if (midiKeyboard)
        return midiKeyboard->keyStateChanged (isKeyDown);

    return false;
}

float PluginEditor::getScaleFactor() const {
    using namespace PluginEditorLayout;

    int availableHeight = getHeight() - getKeyboardStripHeight();

    // Calculate scale based on width (height will be constrained by aspect ratio)
    float scaleX = static_cast<float>(getWidth()) / static_cast<float>(editorWidth);
    float scaleY = static_cast<float>(availableHeight) / static_cast<float>(currentDesignHeight);

    // Use the smaller scale to ensure everything fits
    return juce::jmin (scaleX, scaleY);
}

void PluginEditor::componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) {
    using namespace PluginEditorLayout;

    // Ignore if not resized or if we're already adjusting
    if (!wasResized || isAdjustingSize || &component != this) {
        return;
    }

#if !JUCE_IOS
    // On desktop: enforce aspect ratio by adjusting window size
    // On iOS: let the host control the size (hosts won't allow size changes)

    // Calculate aspect ratio using current design height (changes with compact mode)
    const float aspectRatio = static_cast<float>(editorWidth) / static_cast<float>(currentDesignHeight);

    // Get current size
    int currentWidth = getWidth();
    int currentHeight = getHeight();
    // Calculate what the height should be for the current width (plus keyboard strip if visible)
    int targetHeight = juce::roundToInt (static_cast<float>(currentWidth) / aspectRatio) + getKeyboardStripHeight();

    // If height doesn't match aspect ratio, adjust it
    if (currentHeight != targetHeight) {
        isAdjustingSize = true;
        setSize (currentWidth, targetHeight);
        isAdjustingSize = false;
    }
#endif
}