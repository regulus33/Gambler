#pragma once

#include <vector>
#include <array>
#include "StepButton.h"

/**
 * State class - Central domain state class for the Gambler application.
 * This is a pure domain object that holds all application state.
 * The actual UI controls interface will be closer to the UI layer.
 */
class State {
public:
    // Constants
    static constexpr int NUM_STEP_BUTTONS = 8;
    static constexpr int NUM_CC_SLOTS = 7;
    static constexpr int NUM_SCALE_NOTES = 12;  // 12 chromatic notes
    static constexpr int MIN_OCTAVE = -4;
    static constexpr int MAX_OCTAVE = 4;
    static constexpr int MIN_NOTE = 0;
    static constexpr int MAX_NOTE = 127;
    static constexpr int MIN_PROBABILITY = 0;
    static constexpr int MAX_PROBABILITY = 100;
    static constexpr int MIN_CC_NUMBER = 0;
    static constexpr int MAX_CC_NUMBER = 127;
    static constexpr int MIN_CC_VALUE = 0;
    static constexpr int MAX_CC_VALUE = 127;
    
    // Enum for CC/Note toggle mode
    enum class MidiMode {
        CC_TO_NOTE,
        NOTE_TO_CC
    };
    
    State() {
        // Initialize step buttons
        stepButtons.resize(NUM_STEP_BUTTONS);

        // Initialize CC controls with default values
        for (int i = 0; i < NUM_CC_SLOTS; ++i) {
            ccNumbers[i] = i + 1;  // Default CC 1-7
            ccValues[i] = 64;       // Default middle value
        }
    }
    
    // ========================================
    // Step Buttons (8 buttons above bottom controls)
    // ========================================
    
    std::vector<StepButton>& getStepButtons() { return stepButtons; }
    const std::vector<StepButton>& getStepButtons() const { return stepButtons; }
    
    StepButton& getStepButton(int index) {
        if (index >= 0 && index < NUM_STEP_BUTTONS) {
            return stepButtons[index];
        }
        // Return first button as fallback (could throw exception instead)
        return stepButtons[0];
    }
    
    // ========================================
    // Bottom Row Controls (Left to Right)
    // ========================================
    
    // Roll button
    bool isRollPressed() const { return rollPressed; }
    void setRollPressed(bool value) { rollPressed = value; }
    void toggleRoll() { rollPressed = !rollPressed; }
    
    // Index Left button (decrease index)
    bool isIndexLeftPressed() const { return indexLeftPressed; }
    void setIndexLeftPressed(bool value) { indexLeftPressed = value; }
    
    // Index Right button (increase index)
    bool isIndexRightPressed() const { return indexRightPressed; }
    void setIndexRightPressed(bool value) { indexRightPressed = value; }
    
    // Length Minus button (decrease length)
    bool isLengthMinusPressed() const { return lengthMinusPressed; }
    void setLengthMinusPressed(bool value) { lengthMinusPressed = value; }
    
    // Length Plus button (increase length)
    bool isLengthPlusPressed() const { return lengthPlusPressed; }
    void setLengthPlusPressed(bool value) { lengthPlusPressed = value; }
    
    // Nudge Left button
    bool isNudgeLeftPressed() const { return nudgeLeftPressed; }
    void setNudgeLeftPressed(bool value) { nudgeLeftPressed = value; }
    
    // Nudge Right button
    bool isNudgeRightPressed() const { return nudgeRightPressed; }
    void setNudgeRightPressed(bool value) { nudgeRightPressed = value; }
    
    // Shift button
    bool isShiftPressed() const { return shiftPressed; }
    void setShiftPressed(bool value) { shiftPressed = value; }
    void toggleShift() { shiftPressed = !shiftPressed; }
    
    // ========================================
    // Top Right Controls
    // ========================================
    
    // Export button
    bool isExportPressed() const { return exportPressed; }
    void setExportPressed(bool value) { exportPressed = value; }
    
    // Save button
    bool isSavePressed() const { return savePressed; }
    void setSavePressed(bool value) { savePressed = value; }
    
    // Load button
    bool isLoadPressed() const { return loadPressed; }
    void setLoadPressed(bool value) { loadPressed = value; }
    
    // ========================================
    // Main Controls (Center Top)
    // ========================================
    
    // Octave control
    int getOctave() const { return octave; }
    void setOctave(int value) {
        if (value >= MIN_OCTAVE && value <= MAX_OCTAVE) {
            octave = value;
        }
    }
    
    // Note control
    int getNote() const { return note; }
    void setNote(int value) {
        if (value >= MIN_NOTE && value <= MAX_NOTE) {
            note = value;
        }
    }
    
    // Probability control
    int getProbability() const { return probability; }
    void setProbability(int value) {
        if (value >= MIN_PROBABILITY && value <= MAX_PROBABILITY) {
            probability = value;
        }
    }
    
    // ========================================
    // Sub Notes Controls
    // ========================================
    
    // Sub Notes Plus button
    bool isSubNotesPlusPressed() const { return subNotesPlusPressed; }
    void setSubNotesPlusPressed(bool value) { subNotesPlusPressed = value; }
    
    // Sub Notes Minus button
    bool isSubNotesMinusPressed() const { return subNotesMinusPressed; }
    void setSubNotesMinusPressed(bool value) { subNotesMinusPressed = value; }
    
    // ========================================
    // MIDI Mode Toggle
    // ========================================
    
    MidiMode getMidiMode() const { return midiMode; }
    void setMidiMode(MidiMode mode) { midiMode = mode; }
    void toggleMidiMode() {
        midiMode = (midiMode == MidiMode::CC_TO_NOTE) 
            ? MidiMode::NOTE_TO_CC 
            : MidiMode::CC_TO_NOTE;
    }
    
    // ========================================
    // CC Controls (7 slots)
    // ========================================
    
    // CC Number selectors (combo box values)
    int getCCNumber(int slotIndex) const {
        if (slotIndex >= 0 && slotIndex < NUM_CC_SLOTS) {
            return ccNumbers[slotIndex];
        }
        return 0;
    }
    
    void setCCNumber(int slotIndex, int ccNumber) {
        if (slotIndex >= 0 && slotIndex < NUM_CC_SLOTS) {
            if (ccNumber >= MIN_CC_NUMBER && ccNumber <= MAX_CC_NUMBER) {
                ccNumbers[slotIndex] = ccNumber;
            }
        }
    }
    
    // CC Values (vertical sliders)
    int getCCValue(int slotIndex) const {
        if (slotIndex >= 0 && slotIndex < NUM_CC_SLOTS) {
            return ccValues[slotIndex];
        }
        return 0;
    }
    
    void setCCValue(int slotIndex, int value) {
        if (slotIndex >= 0 && slotIndex < NUM_CC_SLOTS) {
            if (value >= MIN_CC_VALUE && value <= MAX_CC_VALUE) {
                ccValues[slotIndex] = value;
            }
        }
    }
    
    // ========================================
    // Note Range Control (Vertical slider on right)
    // ========================================

    int getNoteRangeMin() const { return noteRangeMin; }
    void setNoteRangeMin(int value) {
        if (value >= MIN_NOTE && value <= MAX_NOTE && value <= noteRangeMax) {
            noteRangeMin = value;
        }
    }

    int getNoteRangeMax() const { return noteRangeMax; }
    void setNoteRangeMax(int value) {
        if (value >= MIN_NOTE && value <= MAX_NOTE && value >= noteRangeMin) {
            noteRangeMax = value;
        }
    }

    void setNoteRange(int min, int max) {
        if (min >= MIN_NOTE && max <= MAX_NOTE && min <= max) {
            noteRangeMin = min;
            noteRangeMax = max;
        }
    }

    // ========================================
    // Scale Control (12 chromatic notes)
    // ========================================

    // Get the scale array (C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B)
    const std::array<int, NUM_SCALE_NOTES>& getScale() const { return scale; }

    // Get individual scale note state (0 = not in scale, 1 = in scale)
    int getScaleNote(int index) const {
        if (index >= 0 && index < NUM_SCALE_NOTES) {
            return scale[index];
        }
        return 0;
    }

    // Set individual scale note state
    void setScaleNote(int index, int value) {
        if (index >= 0 && index < NUM_SCALE_NOTES) {
            scale[index] = (value != 0) ? 1 : 0;  // Normalize to 0 or 1
        }
    }

    // Set entire scale at once (C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B)
    void setScale(int c, int db, int d, int eb, int e, int f, int gb, int g, int ab, int a, int bb, int b) {
        scale[0] = (c != 0) ? 1 : 0;
        scale[1] = (db != 0) ? 1 : 0;
        scale[2] = (d != 0) ? 1 : 0;
        scale[3] = (eb != 0) ? 1 : 0;
        scale[4] = (e != 0) ? 1 : 0;
        scale[5] = (f != 0) ? 1 : 0;
        scale[6] = (gb != 0) ? 1 : 0;
        scale[7] = (g != 0) ? 1 : 0;
        scale[8] = (ab != 0) ? 1 : 0;
        scale[9] = (a != 0) ? 1 : 0;
        scale[10] = (bb != 0) ? 1 : 0;
        scale[11] = (b != 0) ? 1 : 0;
    }
    
    // ========================================
    // Reset Methods
    // ========================================
    
    void resetAllButtons() {
        rollPressed = false;
        indexLeftPressed = false;
        indexRightPressed = false;
        lengthMinusPressed = false;
        lengthPlusPressed = false;
        nudgeLeftPressed = false;
        nudgeRightPressed = false;
        shiftPressed = false;
        exportPressed = false;
        savePressed = false;
        loadPressed = false;
        subNotesPlusPressed = false;
        subNotesMinusPressed = false;
        
        for (auto& button : stepButtons) {
            button.reset();
        }
    }
    
    void resetToDefaults() {
        resetAllButtons();
        octave = 0;
        note = 60;  // Middle C
        probability = 100;
        midiMode = MidiMode::CC_TO_NOTE;
        noteRangeMin = 0;
        noteRangeMax = 127;
        
        for (int i = 0; i < NUM_CC_SLOTS; ++i) {
            ccNumbers[i] = i + 1;
            ccValues[i] = 64;
        }
    }
    
private:
    // Step buttons (8 buttons in sequence grid area)
    std::vector<StepButton> stepButtons;
    
    // Bottom row buttons
    bool rollPressed = false;
    bool indexLeftPressed = false;
    bool indexRightPressed = false;
    bool lengthMinusPressed = false;
    bool lengthPlusPressed = false;
    bool nudgeLeftPressed = false;
    bool nudgeRightPressed = false;
    bool shiftPressed = false;
    
    // Top right buttons
    bool exportPressed = false;
    bool savePressed = false;
    bool loadPressed = false;
    
    // Sub notes buttons
    bool subNotesPlusPressed = false;
    bool subNotesMinusPressed = false;
    
    // Main controls
    int octave = 0;
    int note = 60;  // Middle C
    int probability = 100;
    
    // MIDI mode toggle
    MidiMode midiMode = MidiMode::CC_TO_NOTE;
    
    // CC controls (7 slots)
    std::array<int, NUM_CC_SLOTS> ccNumbers;
    std::array<int, NUM_CC_SLOTS> ccValues;

    // Note range control
    int noteRangeMin = 0;
    int noteRangeMax = 127;

    // Scale control (12 chromatic notes: C, Db, D, Eb, E, F, Gb, G, Ab, A, Bb, B)
    // 0 = not in scale, 1 = in scale
    std::array<int, NUM_SCALE_NOTES> scale = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
};

