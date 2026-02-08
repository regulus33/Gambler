#pragma once

#include "../backend/Scale.h"
#include "../backend/MidiLearnRegistry.h"
#include "BinaryData.h"
#include "MidiLearnOverlayPainter.h"
#include "StateListenerComponent.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace PianoRollConstants {
    // Scale factor to match 4 sequencer steps (4×92px) + 3 gaps (3×18px) = 422px
    constexpr float scaleFactor = 1.031f;
    // Component size (scaled)
    constexpr int componentWidth = 422;
    constexpr int componentHeight = 206;
    // Font
    constexpr float fontSize = 12.4f;  // Scaled 3.1% from 12.0f
    // Active note circle
    constexpr float activeCircleRadius = 13.4f;  // ~27px diameter (scaled from 13.0f)
    // Key border
    constexpr float keyBorderWidth = 1.0f;
    // Black key positioning
    constexpr float blackKeyWidth = 40.0f;
    constexpr float blackKeyShift = 30.0f;
    // Unselected key colors
    const juce::Colour unselectedWhiteKeyColor = juce::Colours::white;
    const juce::Colour unselectedBlackKeyColor = juce::Colour (186, 186, 186);
}

/**
 * KeyData - Data structure for a single piano key
 */
struct KeyData {
    juce::String name;
    juce::Colour color;
    bool isBlack;
    bool isToggled;
    juce::Path path;

    KeyData (const juce::String& n, const juce::Colour& c, bool black)
        : name (n), color (c), isBlack (black), isToggled (true) {}
};

/**
 * PianoRollComponent - Piano keyboard for scale selection
 *
 * Displays 12 keys representing the chromatic scale using custom SVG paths
 * Users can toggle notes on/off to define the scale
 * Total size: 422 x 206 pixels (scaled 3.1% to match 4 sequencer steps + 3 gaps)
 *
 * Observer Pattern:
 * - Listens to all scale parameters (12 notes + range min/max)
 * - Automatically updates UI when scale parameters change
 */
class PianoRollComponent : public StateListenerComponent {
public:
    explicit PianoRollComponent(PluginProcessor& processor)
        : StateListenerComponent(processor) {
        using namespace PianoRollConstants;

        // Load Inter font from binary data
        interFont = juce::Typeface::createSystemTypefaceFor (
            BinaryData::InterVariableFont_ttf,
            BinaryData::InterVariableFont_ttfSize);

        // Note names and colors matching the design
        struct KeyInfo {
            const char* name;
            juce::Colour color;
            bool isBlack;
        };

        const KeyInfo keyInfos[12] = {
            { "C", juce::Colour (0xFFFF6B6B), false }, // Red
            { "Db", juce::Colour (0xFFFF9F6B), true }, // Orange
            { "D", juce::Colour (0xFFFFD66B), false }, // Yellow
            { "Eb", juce::Colour (0xFFFFE66B), true }, // Light yellow
            { "E", juce::Colour (0xFFE6FF6B), false }, // Yellow-green
            { "F", juce::Colour (0xFF6BFFB4), false }, // Light green
            { "Gb", juce::Colour (0xFF6BFFD6), true }, // Cyan-green
            { "G", juce::Colour (0xFF6BD6FF), false }, // Cyan
            { "Ab", juce::Colour (0xFF6BB4FF), true }, // Light blue
            { "A", juce::Colour (0xFF9F6BFF), false }, // Purple
            { "Bb", juce::Colour (0xFFD66BFF), true }, // Magenta
            { "B", juce::Colour (0xFFFF6BD6), false } // Pink
        };

        // Create 12 key data structures
        for (int i = 0; i < 12; i++) {
            keys.add (new KeyData (keyInfos[i].name, keyInfos[i].color, keyInfos[i].isBlack));
        }

        // Initialize MIDI control IDs for each key (SCALE_NOTE_C through SCALE_NOTE_B)
        for (int i = 0; i < 12; i++) {
            keyMidiControlIds_[i] = static_cast<MidiControlId>(
                static_cast<int>(MidiControlId::SCALE_NOTE_C) + i);
        }

        // Set up paths with proper positioning
        setupKeyPaths();

        // Subscribe to all scale parameters
        listenToStepScaleNotes(0, ParamConstants::MAX_STEPS - 1);  // All steps' scale notes
        listenToScaleRange();  // Scale range min/max
        listenToParameter(Params::Keys::ID::SEQ_SELECTED_STEP);  // Selected step changes

        setSize (componentWidth, componentHeight);
    }

    ~PianoRollComponent() override {
        keys.clear (true);
    }

    void paint (juce::Graphics& g) override {
        // Background - dark grey matching the design
        g.fillAll (juce::Colour (0xFF2B2B2B));

        // Draw black keys first (so they appear behind)
        for (int i = 0; i < 12; i++) {
            if (keys[i]->isBlack) {
                drawKey (g, *keys[i], i);
            }
        }

        // Draw white keys on top (they overlap the black keys)
        for (int i = 0; i < 12; i++) {
            if (!keys[i]->isBlack) {
                drawKey (g, *keys[i], i);
            }
        }

        // Draw MIDI learn overlays on top of everything (so borders are fully visible)
        if (registry_ && registry_->isLearning()) {
            for (int i = 0; i < 12; i++) {
                paintMidiLearnOverlayForKey(g, keys[i]->path.getBounds(), i);
            }
        }
    }

    void mouseDown (const juce::MouseEvent& event) override {
        // Check if scale latch is enabled
        bool isLatched = isScaleLatchEnabled();

        // Check white keys first (they're on top visually)
        for (int i = 0; i < 12; i++) {
            if (!keys[i]->isBlack && keys[i]->path.contains (event.position)) {
                // Handle MIDI learn mode first
                if (registry_ && registry_->isLearning() && keyMidiControlIds_[i] != MidiControlId::COUNT) {
                    registry_->setLearningControl(keyMidiControlIds_[i]);
                    repaint();
                    return;
                }

                // Store the pressed key for momentary mode
                pressedKeyIndex_ = i;

                if (isLatched) {
                    // Latched mode: toggle the key on/off
                    if (tryToggleKey(i)) {
                        repaint();
                        if (onNoteChanged)
                            onNoteChanged();
                    }
                } else {
                    // Momentary mode: turn the key ON
                    if (trySetKey(i, true)) {
                        repaint();
                        if (onNoteChanged)
                            onNoteChanged();
                    }
                }
                return;
            }
        }

        // Then check black keys (only if click wasn't on a white key)
        for (int i = 0; i < 12; i++) {
            if (keys[i]->isBlack && keys[i]->path.contains (event.position)) {
                // Handle MIDI learn mode first
                if (registry_ && registry_->isLearning() && keyMidiControlIds_[i] != MidiControlId::COUNT) {
                    registry_->setLearningControl(keyMidiControlIds_[i]);
                    repaint();
                    return;
                }

                // Store the pressed key for momentary mode
                pressedKeyIndex_ = i;

                if (isLatched) {
                    // Latched mode: toggle the key on/off
                    if (tryToggleKey(i)) {
                        repaint();
                        if (onNoteChanged)
                            onNoteChanged();
                    }
                } else {
                    // Momentary mode: turn the key ON
                    if (trySetKey(i, true)) {
                        repaint();
                        if (onNoteChanged)
                            onNoteChanged();
                    }
                }
                return;
            }
        }
    }

    void mouseUp (const juce::MouseEvent& event) override {
        // In momentary mode, turn the key OFF when mouse is released
        if (!isScaleLatchEnabled() && pressedKeyIndex_ >= 0) {
            if (trySetKey(pressedKeyIndex_, false)) {
                repaint();
                if (onNoteChanged)
                    onNoteChanged();
            }
            pressedKeyIndex_ = -1;
        }
    }

    void resized() override {
        // Nothing to do - we handle all rendering in paint()
    }

    /**
     * Set the scale (which notes are active)
     */
    void setScale (const Scale& scale) {
        auto scaleNotes = scale.getScaleNotes();
        for (int i = 0; i < 12; i++) {
            keys[i]->isToggled = (scaleNotes[i] != 0);
        }
        repaint();
    }

    /**
     * Get the current scale
     */
    Scale getScale() const {
        std::array<int, 12> scaleNotes;
        for (int i = 0; i < 12; i++) {
            scaleNotes[i] = keys[i]->isToggled ? 1 : 0;
        }

        Scale scale;
        scale.setScaleNotes (scaleNotes);
        return scale;
    }

    /**
     * Set the active note (0-11 for chromatic scale, -1 for none)
     * This highlights the currently playing note in the sequence
     */
    void setActiveNote (int noteIndex) {
        if (activeNoteIndex != noteIndex) {
            activeNoteIndex = noteIndex;
            repaint();
        }
    }

    /**
     * Get the active note index (-1 if none)
     */
    int getActiveNote() const {
        return activeNoteIndex;
    }

    /**
     * Get the indices of all currently selected (toggled) notes
     * @return Array of note indices (0-11) that are currently toggled on
     */
    juce::Array<int> getSelectedNoteIndices() const {
        juce::Array<int> selectedIndices;
        for (int i = 0; i < 12; i++) {
            if (keys[i]->isToggled) {
                selectedIndices.add(i);
            }
        }
        return selectedIndices;
    }

    /**
     * Get the names of all currently selected (toggled) notes
     * @return Array of note names (e.g., "C", "D", "E") that are currently toggled on
     */
    juce::Array<juce::String> getSelectedNoteNames() const {
        juce::Array<juce::String> selectedNames;
        for (int i = 0; i < 12; i++) {
            if (keys[i]->isToggled) {
                selectedNames.add(keys[i]->name);
            }
        }
        return selectedNames;
    }

    /**
     * Get the colors of all currently selected (toggled) notes
     * @return Array of colors corresponding to the selected notes
     */
    juce::Array<juce::Colour> getSelectedNoteColours() const {
        juce::Array<juce::Colour> selectedColours;
        for (int i = 0; i < 12; i++) {
            if (keys[i]->isToggled) {
                selectedColours.add(keys[i]->color);
            }
        }
        return selectedColours;
    }

    /**
     * Simulate clicking a key (for testing)
     * @param keyIndex The index of the key to click (0-11)
     * @return true if the key was toggled, false if action was prevented
     */
    bool TEST_simulateKeyClick(int keyIndex) {
        if (keyIndex < 0 || keyIndex >= 12) return false;

        bool wasToggled = tryToggleKey(keyIndex);
        if (wasToggled) {
            repaint();
            if (onNoteChanged) {
                onNoteChanged();
            }
        }
        return wasToggled;
    }

    /**
     * Get key data for testing/inspection
     * @param keyIndex The index of the key (0-11)
     * @return Pointer to KeyData, or nullptr if invalid index
     */
    const KeyData* getKeyData(int keyIndex) const {
        if (keyIndex >= 0 && keyIndex < 12) {
            return keys[keyIndex];
        }
        return nullptr;
    }

    /**
     * Callback when a note is toggled
     */
    std::function<void()> onNoteChanged;

    /**
     * Set the MIDI learn registry for this component
     */
    void setRegistry(MidiLearnRegistry* registry) {
        registry_ = registry;
    }

protected:
    /**
     * Called on message thread when any subscribed parameter changes.
     * Updates the piano roll to reflect the current scale state.
     */
    void onStateChanged() override {
        // Read scale from the selected step's APVTS state
        auto& state = getState();
        int selectedStep = state.getSelectedStepForEditing();
        if (selectedStep < 0) return;

        Scale scale = state.getStepScale(selectedStep);

        // Update key toggle states
        auto scaleNotes = scale.getScaleNotes();
        for (int i = 0; i < 12; i++) {
            keys[i]->isToggled = (scaleNotes[i] != 0);
        }

        repaint();
    }

private:
    /**
     * Count how many notes are currently toggled on
     */
    int countToggledNotes() const {
        int count = 0;
        for (int i = 0; i < 12; i++) {
            if (keys[i]->isToggled)
                count++;
        }
        return count;
    }

    /**
     * Try to toggle a key - returns true if toggle was successful
     */
    bool tryToggleKey(int keyIndex) {
        keys[keyIndex]->isToggled = !keys[keyIndex]->isToggled;
        return true;
    }

    /**
     * Try to set a key to a specific state - returns true if state was changed
     */
    bool trySetKey(int keyIndex, bool enabled) {
        if (keys[keyIndex]->isToggled != enabled) {
            keys[keyIndex]->isToggled = enabled;
            return true;
        }
        return false;
    }

    /**
     * Check if scale latch mode is enabled
     */
    bool isScaleLatchEnabled() const {
        if (auto* param = getProcessor().getAPVTS().getParameter(Params::Keys::ID::SCALE_LATCH)) {
            return param->getValue() > 0.5f;
        }
        return false;  // Default to momentary mode
    }

    /**
     * Draw a single key
     */
    void drawKey (juce::Graphics& g, const KeyData& key, int keyIndex) {
        using namespace PianoRollConstants;

        // Fill the key with color
        if (key.isToggled) {
            // Selected keys: use their assigned color
            g.setColour (key.color);
        } else {
            // Unselected keys: white for white keys, grey for black keys
            g.setColour (key.isBlack ? unselectedBlackKeyColor : unselectedWhiteKeyColor);
        }
        g.fillPath (key.path);

        // Draw border
        g.setColour (juce::Colours::black);
        g.strokePath (key.path, juce::PathStrokeType (keyBorderWidth));

        // Get the bounds of the path for text positioning
        auto pathBounds = key.path.getBounds();

        // Determine text area based on key type
        auto textArea = pathBounds.withTrimmedTop (pathBounds.getHeight() * 0.7f);

        // Check if this is the active note
        bool isActive = (keyIndex == activeNoteIndex);

        if (isActive) {
            // Draw black circle behind the text
            auto circleCentre = textArea.getCentre();
            g.setColour (juce::Colours::black);
            g.fillEllipse (circleCentre.x - activeCircleRadius, circleCentre.y - activeCircleRadius,
                          activeCircleRadius * 2.0f, activeCircleRadius * 2.0f);

            // Draw note name in white, bold Inter font
            g.setColour (juce::Colours::white);
            juce::Font font (interFont);
            font.setHeight (fontSize);
            font.setBold (true);
            g.setFont (font);
        } else {
            // Draw note name in black, regular Inter font
            g.setColour (juce::Colours::black);
            juce::Font font (interFont);
            font.setHeight (fontSize);
            g.setFont (font);
        }

        // Draw the text
        g.drawText (key.name, textArea.toNearestInt(), juce::Justification::centred);
    }

    /**
     * Setup paths for all 12 keys with proper horizontal positioning
     * Each key path is offset to its correct position in the piano roll
     *
     * The positions are calculated based on the actual SVG path widths:
     * - C/F keys (right notch): 57.1 pixels wide (0.5 to 57.1)
     * - D/G/A keys (both notches): 60.7 pixels wide (0.5 to 60.7)
     * - E/B keys (left notch): 57.1 pixels wide (0.5 to 57.1)
     * - Black keys: ~40 pixels wide
     *
     * Black keys are centered between white keys for proper piano layout
     */
    void setupKeyPaths() {
        using namespace PianoRollConstants;

        // Calculate positions by accumulating the actual widths
        float xPos = 0.0f;

        // C key (index 0) - width 57.1
        float cWidth = 57.1f - 0.5f;
        keys[0]->path = createWhiteKeyRightNotch (xPos);
        xPos += cWidth;

        // Db key - centered between C and D
        float dWidth = 60.7f - 0.5f;
        float dbOffset = xPos - (cWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift;
        keys[1]->path = createBlackKey (dbOffset);

        // D key (index 2) - width 60.7
        keys[2]->path = createWhiteKeyBothNotches (xPos);
        xPos += dWidth;

        // Eb key - centered between D and E
        float eWidth = 57.1f - 0.5f;
        float ebOffset = xPos - (dWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift;
        keys[3]->path = createBlackKey (ebOffset);

        // E key (index 4) - width 57.1
        keys[4]->path = createWhiteKeyLeftNotch (xPos);
        xPos += eWidth;

        // F key (index 5) - width 57.1
        float fWidth = 57.1f - 0.5f;
        keys[5]->path = createWhiteKeyRightNotch (xPos);
        xPos += fWidth;

        // Gb key - centered between F and G
        float gWidth = 60.7f - 0.5f;
        float gbOffset = xPos - (fWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift;
        keys[6]->path = createBlackKey (gbOffset);

        // G key (index 7) - width 60.7
        keys[7]->path = createWhiteKeyBothNotches (xPos);
        xPos += gWidth;

        // Ab key - centered between G and A
        float aWidth = 60.7f - 0.5f;
        float abOffset = xPos - (gWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift;
        keys[8]->path = createBlackKey (abOffset);

        // A key (index 9) - width 60.7
        keys[9]->path = createWhiteKeyBothNotches (xPos);
        xPos += aWidth;

        // Bb key - centered between A and B
        float bWidth = 57.1f - 0.5f;
        float bbOffset = xPos - (aWidth / 2.0f) - (blackKeyWidth / 2.0f) + blackKeyShift;
        keys[10]->path = createBlackKey (bbOffset);

        // B key (index 11) - width 57.1
        keys[11]->path = createWhiteKeyLeftNotch (xPos);

        // Apply scale transform to all paths to match sequencer step width
        auto scaleTransform = juce::AffineTransform::scale (scaleFactor);
        for (int i = 0; i < 12; i++) {
            keys[i]->path.applyTransform (scaleTransform);
        }
    }

    /**
     * Create white key path with notch on right side (C, F)
     * @param xOffset Horizontal offset for this key
     */
    juce::Path createWhiteKeyRightNotch (float xOffset) {
        juce::Path path;
        path.startNewSubPath (xOffset + 57.1f, 127.7f);
        path.cubicTo (xOffset + 57.1f, 124.9f, xOffset + 54.9f, 122.7f, xOffset + 52.1f, 122.7f);
        path.lineTo (xOffset + 46.3f, 122.7f);
        path.cubicTo (xOffset + 43.6f, 122.7f, xOffset + 41.3f, 120.4f, xOffset + 41.3f, 117.7f);
        path.lineTo (xOffset + 41.3f, 5.5f);
        path.cubicTo (xOffset + 41.3f, 2.7f, xOffset + 39.1f, 0.5f, xOffset + 36.3f, 0.5f);
        path.lineTo (xOffset + 5.5f, 0.5f);
        path.cubicTo (xOffset + 2.7f, 0.5f, xOffset + 0.5f, 2.7f, xOffset + 0.5f, 5.5f);
        path.lineTo (xOffset + 0.5f, 194.5f);
        path.cubicTo (xOffset + 0.5f, 197.3f, xOffset + 2.7f, 199.5f, xOffset + 5.5f, 199.5f);
        path.lineTo (xOffset + 52.1f, 199.5f);
        path.cubicTo (xOffset + 54.9f, 199.5f, xOffset + 57.1f, 197.3f, xOffset + 57.1f, 194.5f);
        path.lineTo (xOffset + 57.1f, 127.7f);
        path.closeSubPath();
        return path;
    }

    /**
     * Create black key path
     * @param xOffset Horizontal offset for this key
     */
    juce::Path createBlackKey (float xOffset) {
        juce::Path path;
        path.startNewSubPath (xOffset + 5.5f, 0.5f);
        path.cubicTo (xOffset + 2.7f, 0.5f, xOffset + 0.5f, 2.8f, xOffset + 0.5f, 5.5f);
        path.lineTo (xOffset + 1.1f, 117.5f);
        path.cubicTo (xOffset + 1.1f, 120.2f, xOffset + 3.3f, 122.4f, xOffset + 5.9f, 122.5f);
        path.lineTo (xOffset + 18.1f, 122.9f);
        path.lineTo (xOffset + 23.7f, 122.9f);
        path.cubicTo (xOffset + 23.7f, 122.9f, xOffset + 23.7f, 122.9f, xOffset + 23.7f, 122.8f);
        path.cubicTo (xOffset + 23.7f, 122.8f, xOffset + 23.7f, 122.8f, xOffset + 23.8f, 122.8f);
        path.lineTo (xOffset + 35.5f, 122.8f);
        path.cubicTo (xOffset + 38.3f, 122.8f, xOffset + 40.5f, 120.6f, xOffset + 40.5f, 117.8f);
        path.lineTo (xOffset + 40.5f, 5.5f);
        path.cubicTo (xOffset + 40.5f, 2.7f, xOffset + 38.3f, 0.5f, xOffset + 35.5f, 0.5f);
        path.lineTo (xOffset + 5.5f, 0.5f);
        path.closeSubPath();
        return path;
    }

    /**
     * Create white key path with notches on both sides (D, G, A)
     * @param xOffset Horizontal offset for this key
     */
    juce::Path createWhiteKeyBothNotches (float xOffset) {
        juce::Path path;
        path.startNewSubPath (xOffset + 60.7f, 127.7f);
        path.cubicTo (xOffset + 60.7f, 124.9f, xOffset + 58.4f, 122.7f, xOffset + 55.7f, 122.7f);
        path.lineTo (xOffset + 47.5f, 122.7f);
        path.cubicTo (xOffset + 44.7f, 122.7f, xOffset + 42.5f, 120.4f, xOffset + 42.5f, 117.7f);
        path.lineTo (xOffset + 42.5f, 5.5f);
        path.cubicTo (xOffset + 42.5f, 2.7f, xOffset + 40.3f, 0.5f, xOffset + 37.5f, 0.5f);
        path.lineTo (xOffset + 23.7f, 0.5f);
        path.cubicTo (xOffset + 20.9f, 0.5f, xOffset + 18.7f, 2.7f, xOffset + 18.7f, 5.5f);
        path.lineTo (xOffset + 18.7f, 117.7f);
        path.cubicTo (xOffset + 18.7f, 120.4f, xOffset + 16.5f, 122.7f, xOffset + 13.7f, 122.7f);
        path.lineTo (xOffset + 5.5f, 122.7f);
        path.cubicTo (xOffset + 2.7f, 122.7f, xOffset + 0.5f, 124.9f, xOffset + 0.5f, 127.7f);
        path.lineTo (xOffset + 0.5f, 194.5f);
        path.cubicTo (xOffset + 0.5f, 197.3f, xOffset + 2.7f, 199.5f, xOffset + 5.5f, 199.5f);
        path.lineTo (xOffset + 55.7f, 199.5f);
        path.cubicTo (xOffset + 58.4f, 199.5f, xOffset + 60.7f, 197.3f, xOffset + 60.7f, 194.5f);
        path.lineTo (xOffset + 60.7f, 127.7f);
        path.closeSubPath();
        return path;
    }

    /**
     * Create white key path with notch on left side (E, B)
     * @param xOffset Horizontal offset for this key
     */
    juce::Path createWhiteKeyLeftNotch (float xOffset) {
        juce::Path path;
        path.startNewSubPath (xOffset + 0.5f, 127.7f);
        path.cubicTo (xOffset + 0.5f, 124.9f, xOffset + 2.7f, 122.7f, xOffset + 5.5f, 122.7f);
        path.lineTo (xOffset + 11.3f, 122.7f);
        path.cubicTo (xOffset + 14.1f, 122.7f, xOffset + 16.3f, 120.4f, xOffset + 16.3f, 117.7f);
        path.lineTo (xOffset + 16.3f, 5.5f);
        path.cubicTo (xOffset + 16.3f, 2.7f, xOffset + 18.5f, 0.5f, xOffset + 21.3f, 0.5f);
        path.lineTo (xOffset + 52.1f, 0.5f);
        path.cubicTo (xOffset + 54.9f, 0.5f, xOffset + 57.1f, 2.7f, xOffset + 57.1f, 5.5f);
        path.lineTo (xOffset + 57.1f, 194.5f);
        path.cubicTo (xOffset + 57.1f, 197.3f, xOffset + 54.9f, 199.5f, xOffset + 52.1f, 199.5f);
        path.lineTo (xOffset + 5.5f, 199.5f);
        path.cubicTo (xOffset + 2.7f, 199.5f, xOffset + 0.5f, 197.3f, xOffset + 0.5f, 194.5f);
        path.lineTo (xOffset + 0.5f, 127.7f);
        path.closeSubPath();
        return path;
    }

    /**
     * Paint MIDI learn overlay for a specific key
     */
    void paintMidiLearnOverlayForKey(juce::Graphics& g, juce::Rectangle<float> keyBounds, int keyIndex) {
        // No corner radius for piano keys (they have custom shapes)
        // Position MIDI mapping text in bottom 30% of key (same area as note names)
        MidiLearnOverlayPainter::paint(g, keyBounds, registry_, keyMidiControlIds_[keyIndex], 0.0f, 0.3f);
    }

    juce::OwnedArray<KeyData> keys;
    juce::Typeface::Ptr interFont;
    int activeNoteIndex = -1; // -1 means no active note, 0-11 for chromatic scale

    // MIDI learn support
    MidiLearnRegistry* registry_ = nullptr;
    std::array<MidiControlId, 12> keyMidiControlIds_;

    // Momentary key press tracking
    int pressedKeyIndex_ = -1;  // -1 means no key is pressed, 0-11 for chromatic scale

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollComponent)
};
