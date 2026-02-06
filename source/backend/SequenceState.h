#pragma once

#include "../data/Params.h"
#include "Scale.h"
#include "ModulationType.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>
#include <random>

/**
 * StepModulationData - Lightweight struct for step modulation data
 * Used by SequenceState::getSelectedModulations()
 */
struct StepModulationData {
    ModulationType type = ModulationType::CC;
    int ccNumber = 1;
    int value = 64;
    bool selected = false;

    ModulationType getType() const { return type; }
    int getCCNumber() const { return ccNumber; }
    int getValue() const { return value; }
    bool isSelected() const { return selected; }
};

/**
 * SequenceState - APVTS-backed sequencer state accessor
 * 
 * This class provides the same interface as the old Sequence class but reads/writes
 * directly to APVTS parameters. This makes all state access thread-safe and enables
 * automatic DAW automation support.
 * 
 * Key benefits:
 * - Thread-safe: APVTS parameters use atomics internally
 * - No command queue needed: UI can set parameters directly
 * - DAW automation: All parameters can be automated
 * - MIDI mapping: Parameters can be mapped to MIDI controllers
 * 
 * Usage:
 *   SequenceState state(apvts);
 *   int note = state.getStepNote(0, 0);  // Read note for step 0, substep 0
 *   state.setStepNote(0, 0, 72);         // Set note (thread-safe)
 */
class SequenceState {
public:
    using APVTS = juce::AudioProcessorValueTreeState;
    
    explicit SequenceState(APVTS& apvts) : apvts(apvts) {}
    
    // ========================================
    // SEQUENCE METADATA
    // ========================================
    
    int getLength() const {
        return getIntParam(Params::Keys::ID::SEQ_LENGTH);
    }
    
    void setLength(int length) {
        setIntParam(Params::Keys::ID::SEQ_LENGTH, 
            juce::jlimit(1, ParamConstants::MAX_STEPS, length));
    }
    
    int getCurrentIndex() const {
        return getIntParam(Params::Keys::ID::SEQ_CURRENT_INDEX);
    }
    
    void setCurrentIndex(int index) {
        setIntParam(Params::Keys::ID::SEQ_CURRENT_INDEX, 
            juce::jlimit(-1, getLength() - 1, index));
    }
    
    int getSelectedStepForEditing() const {
        return getIntParam(Params::Keys::ID::SEQ_SELECTED_STEP);
    }
    
    void setSelectedStepForEditing(int index) {
        setIntParam(Params::Keys::ID::SEQ_SELECTED_STEP, 
            juce::jlimit(0, ParamConstants::MAX_STEPS - 1, index));
    }
    
    bool getIsRandom() const {
        return getBoolParam(Params::Keys::ID::SEQ_IS_RANDOM);
    }
    
    void setIsRandom(bool random) {
        setBoolParam(Params::Keys::ID::SEQ_IS_RANDOM, random);
    }
    
    bool getIsShift() const {
        return getBoolParam(Params::Keys::ID::SEQ_IS_SHIFT);
    }
    
    void setIsShift(bool shift) {
        setBoolParam(Params::Keys::ID::SEQ_IS_SHIFT, shift);
    }
    
    int getLastProbabilityIndex() const {
        return getIntParam(Params::Keys::ID::SEQ_LAST_PROB_INDEX);
    }
    
    void setLastProbabilityIndex(int index) {
        setIntParam(Params::Keys::ID::SEQ_LAST_PROB_INDEX, index);
    }

    int getViewWindowStart() const {
        return getIntParam(Params::Keys::ID::SEQ_VIEW_WINDOW_START);
    }

    void setViewWindowStart(int startIndex) {
        setIntParam(Params::Keys::ID::SEQ_VIEW_WINDOW_START,
            juce::jlimit(0, 56, startIndex));
    }

    // ========================================
    // PLAY PROBABILITY
    // ========================================
    
    bool getPlayProbability(int index) const {
        if (index < 0 || index >= ParamConstants::MAX_STEPS) return true;
        return getBoolParam(Params::Keys::ID::playProbability(index));
    }
    
    void setPlayProbability(int index, bool shouldPlay) {
        if (index >= 0 && index < ParamConstants::MAX_STEPS) {
            setBoolParam(Params::Keys::ID::playProbability(index), shouldPlay);
        }
    }
    
    void setProbabilityPercentage(int percentage) {
        percentage = juce::jlimit(0, 100, percentage);
        int numToPlay = (ParamConstants::MAX_STEPS * percentage) / 100;
        
        // Reset all to false
        for (int i = 0; i < ParamConstants::MAX_STEPS; i++) {
            setPlayProbability(i, false);
        }
        
        // Randomly set numToPlay indices to true
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        std::vector<int> indices;
        for (int i = 0; i < ParamConstants::MAX_STEPS; i++) {
            indices.push_back(i);
        }
        std::shuffle(indices.begin(), indices.end(), gen);
        
        for (int i = 0; i < numToPlay && i < static_cast<int>(indices.size()); i++) {
            setPlayProbability(indices[i], true);
        }
    }
    
    // ========================================
    // STEP NOTES
    // ========================================
    
    int getStepNote(int stepIndex, int substepIndex) const {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return ParamConstants::DEFAULT_NOTE;
        if (substepIndex < 0 || substepIndex >= ParamConstants::MAX_SUBSTEPS) return ParamConstants::DEFAULT_NOTE;
        return getIntParam(Params::Keys::ID::stepNote(stepIndex, substepIndex));
    }
    
    void setStepNote(int stepIndex, int substepIndex, int noteNumber) {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
        if (substepIndex < 0 || substepIndex >= ParamConstants::MAX_SUBSTEPS) return;
        setIntParam(Params::Keys::ID::stepNote(stepIndex, substepIndex), 
            juce::jlimit(-1, 127, noteNumber));
    }
    
    int getStepNotesLength(int stepIndex) const {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return 1;
        return getIntParam(Params::Keys::ID::stepNotesLength(stepIndex));
    }
    
    void setStepNotesLength(int stepIndex, int length) {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
        setIntParam(Params::Keys::ID::stepNotesLength(stepIndex),
            juce::jlimit(1, ParamConstants::MAX_SUBSTEPS, length));
    }

    int getStepNotesCurrentIndex(int stepIndex) const {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return -1;
        return getIntParam(Params::Keys::ID::stepNotesCurrentIndex(stepIndex));
    }

    void setStepNotesCurrentIndex(int stepIndex, int index) {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
        setIntParam(Params::Keys::ID::stepNotesCurrentIndex(stepIndex),
            juce::jlimit(-1, ParamConstants::MAX_SUBSTEPS - 1, index));
    }

    /**
     * Get the RAW stored note for a step (based on its current substep index)
     * This returns the unquantized value as stored in APVTS
     * Use getQuantizedCurrentNoteForStep() for display/playback
     */
    int getRawCurrentNoteForStep(int stepIndex) const {
        int substepIdx = getStepNotesCurrentIndex(stepIndex);
        if (substepIdx < 0) substepIdx = 0;
        return getStepNote(stepIndex, substepIdx);
    }

    /**
     * Get the quantized version of a step's note
     * This applies the current scale quantization on-the-fly without modifying stored data
     */
    int getQuantizedStepNote(int stepIndex, int substepIndex) const {
        int note = getStepNote(stepIndex, substepIndex);
        if (note < 0) return note;  // Don't quantize rests
        return quantizeNote(note);
    }

    /**
     * Get the quantized current note for a step (based on its current substep index)
     * This is what should be displayed and played
     */
    int getQuantizedCurrentNoteForStep(int stepIndex) const {
        int substepIdx = getStepNotesCurrentIndex(stepIndex);
        if (substepIdx < 0) substepIdx = 0;
        return getQuantizedStepNote(stepIndex, substepIdx);
    }

    /**
     * Get the current note for a step (based on its current substep index)
     * Returns the QUANTIZED note for display/editing purposes
     * This is the main accessor that UI components should use
     */
    int getCurrentNoteForStep(int stepIndex) const {
        return getQuantizedCurrentNoteForStep(stepIndex);
    }

    /**
     * Update the current note for a step (at its current substep index)
     * Stores the exact value provided (typically from manual user edits)
     */
    void updateCurrentNoteForStep(int stepIndex, int noteNumber) {
        int substepIdx = getStepNotesCurrentIndex(stepIndex);
        if (substepIdx < 0) substepIdx = 0;
        setStepNote(stepIndex, substepIdx, noteNumber);
    }

    /**
     * Check if a step's current note is a rest (-1)
     * Checks the RAW stored value (before quantization)
     */
    bool stepHasRest(int stepIndex) const {
        return getRawCurrentNoteForStep(stepIndex) < 0;
    }

    // ========================================
    // STEP MODULATIONS
    // ========================================

    ModulationType getStepModType(int stepIndex, int modIndex) const {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return ModulationType::CC;
        if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return ModulationType::CC;
        return static_cast<ModulationType>(getIntParam(Params::Keys::ID::stepModType(stepIndex, modIndex)));
    }

    void setStepModType(int stepIndex, int modIndex, ModulationType type) {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
        if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return;
        setIntParam(Params::Keys::ID::stepModType(stepIndex, modIndex), static_cast<int>(type));
    }

    int getStepModCCNum(int stepIndex, int modIndex) const {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return 0;
        if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return 0;
        return getIntParam(Params::Keys::ID::stepModCCNum(stepIndex, modIndex));
    }

    void setStepModCCNum(int stepIndex, int modIndex, int ccNum) {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
        if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return;
        setIntParam(Params::Keys::ID::stepModCCNum(stepIndex, modIndex), juce::jlimit(0, 127, ccNum));
    }

    int getStepModValue(int stepIndex, int modIndex) const {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return 64;
        if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return 64;
        return getIntParam(Params::Keys::ID::stepModValue(stepIndex, modIndex));
    }

    void setStepModValue(int stepIndex, int modIndex, int value) {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
        if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return;
        setIntParam(Params::Keys::ID::stepModValue(stepIndex, modIndex), juce::jlimit(0, 127, value));
    }

    bool getStepModSelected(int stepIndex, int modIndex) const {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return false;
        if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return false;
        return getBoolParam(Params::Keys::ID::stepModSelected(stepIndex, modIndex));
    }

    void setStepModSelected(int stepIndex, int modIndex, bool selected) {
        if (stepIndex < 0 || stepIndex >= ParamConstants::MAX_STEPS) return;
        if (modIndex < 0 || modIndex >= ParamConstants::NUM_MODULATIONS) return;
        setBoolParam(Params::Keys::ID::stepModSelected(stepIndex, modIndex), selected);
    }

    /**
     * Get all selected modulations for a step
     */
    std::vector<StepModulationData> getSelectedModulations(int stepIndex) const {
        std::vector<StepModulationData> result;
        for (int i = 0; i < ParamConstants::NUM_MODULATIONS; i++) {
            if (getStepModSelected(stepIndex, i)) {
                StepModulationData mod;
                mod.type = getStepModType(stepIndex, i);
                mod.ccNumber = getStepModCCNum(stepIndex, i);
                mod.value = getStepModValue(stepIndex, i);
                mod.selected = true;
                result.push_back(mod);
            }
        }
        return result;
    }

    // ========================================
    // SCALE
    // ========================================

    bool getScaleNote(int noteIndex) const {
        if (noteIndex < 0 || noteIndex >= ParamConstants::NUM_SCALE_NOTES) return true;
        return getBoolParam(Params::Keys::ID::scaleNote(noteIndex));
    }

    void setScaleNote(int noteIndex, bool enabled) {
        if (noteIndex >= 0 && noteIndex < ParamConstants::NUM_SCALE_NOTES) {
            setBoolParam(Params::Keys::ID::scaleNote(noteIndex), enabled);
        }
    }

    std::array<int, 12> getScaleNotes() const {
        std::array<int, 12> notes;
        for (int i = 0; i < 12; i++) {
            notes[i] = getScaleNote(i) ? 1 : 0;
        }
        return notes;
    }

    void setScaleNotes(const std::array<int, 12>& notes) {
        for (int i = 0; i < 12; i++) {
            setScaleNote(i, notes[i] != 0);
        }
    }

    int getScaleRangeMin() const {
        return getIntParam(Params::Keys::ID::SCALE_RANGE_MIN);
    }

    void setScaleRangeMin(int min) {
        setIntParam(Params::Keys::ID::SCALE_RANGE_MIN, juce::jlimit(0, 127, min));
    }

    int getScaleRangeMax() const {
        return getIntParam(Params::Keys::ID::SCALE_RANGE_MAX);
    }

    void setScaleRangeMax(int max) {
        setIntParam(Params::Keys::ID::SCALE_RANGE_MAX, juce::jlimit(0, 127, max));
    }

    /**
     * Get a Scale object populated from the current state
     */
    Scale getScale() const {
        Scale scale;
        scale.setScaleNotes(getScaleNotes());
        scale.setRangeMin(getScaleRangeMin());
        scale.setRangeMax(getScaleRangeMax());
        return scale;
    }

    /**
     * Check if a note is in the current scale
     */
    bool isNoteInScale(int noteNumber) const {
        if (noteNumber < 0 || noteNumber > 127) return false;
        int pitchClass = noteNumber % 12;
        return getScaleNote(pitchClass);
    }

    /**
     * Quantize a note to the current scale
     */
    int quantizeNote(int noteNumber) const {
        if (noteNumber < 0) noteNumber = 0;
        if (noteNumber > 127) noteNumber = 127;

        // Check if any notes are enabled
        bool anyEnabled = false;
        for (int i = 0; i < 12; i++) {
            if (getScaleNote(i)) {
                anyEnabled = true;
                break;
            }
        }
        if (!anyEnabled) return noteNumber;

        // If already in scale, return it
        if (isNoteInScale(noteNumber)) return noteNumber;

        // Find nearest note in scale (prefer lower when equidistant)
        for (int distance = 1; distance <= 12; distance++) {
            int downNote = noteNumber - distance;
            if (downNote >= 0 && isNoteInScale(downNote)) return downNote;

            int upNote = noteNumber + distance;
            if (upNote <= 127 && isNoteInScale(upNote)) return upNote;
        }

        return noteNumber;
    }

    // ========================================
    // CLOCK
    // ========================================

    int getClockMode() const {
        return getIntParam(Params::Keys::ID::CLOCK_MODE);
    }

    void setClockMode(int mode) {
        setIntParam(Params::Keys::ID::CLOCK_MODE, juce::jlimit(0, 1, mode));
    }

    float getClockBpm() const {
        return getFloatParam(Params::Keys::ID::CLOCK_BPM);
    }

    void setClockBpm(float bpm) {
        setFloatParam(Params::Keys::ID::CLOCK_BPM, juce::jlimit(20.0f, 999.0f, bpm));
    }

    int getClockSubdivision() const {
        return getIntParam(Params::Keys::ID::CLOCK_SUBDIVISION);
    }

    void setClockSubdivision(int subdivision) {
        setIntParam(Params::Keys::ID::CLOCK_SUBDIVISION, juce::jlimit(0, 3, subdivision));
    }

    // ========================================
    // USER PREFERENCES
    // ========================================

    bool getPreviewNotesOnEdit() const {
        return getBoolParam(Params::Keys::ID::PREF_PREVIEW_NOTES);
    }

    void setPreviewNotesOnEdit(bool preview) {
        setBoolParam(Params::Keys::ID::PREF_PREVIEW_NOTES, preview);
    }

    bool getFollowStep() const {
        return getBoolParam(Params::Keys::ID::PREF_FOLLOW_STEP);
    }

    void setFollowStep(bool follow) {
        setBoolParam(Params::Keys::ID::PREF_FOLLOW_STEP, follow);
    }

    bool getFollowPlayback() const {
        return getBoolParam(Params::Keys::ID::PREF_FOLLOW_PLAYBACK);
    }

    void setFollowPlayback(bool follow) {
        setBoolParam(Params::Keys::ID::PREF_FOLLOW_PLAYBACK, follow);
    }

    float getNoteLength() const {
        return getFloatParam(Params::Keys::ID::PREF_NOTE_LENGTH);
    }

    void setNoteLength(float length) {
        setFloatParam(Params::Keys::ID::PREF_NOTE_LENGTH, juce::jlimit(0.0f, 1.0f, length));
    }

    bool getPianoKeyMomentary() const {
        return getBoolParam(Params::Keys::ID::PREF_PIANO_KEY_MOMENTARY);
    }

    void setPianoKeyMomentary(bool momentary) {
        setBoolParam(Params::Keys::ID::PREF_PIANO_KEY_MOMENTARY, momentary);
    }

    // ========================================
    // SEQUENCER OPERATIONS
    // ========================================

    /**
     * Advance the sequencer to the next step and return the note to play.
     * This is the main tick function called from processBlock.
     *
     * @return MIDI note number to play (quantized), or -1 for rest
     */
    int tick() {
        int length = getLength();
        int currentIndex = getCurrentIndex();

        // Advance index (wraps at length boundary)
        currentIndex = (currentIndex + 1) % length;
        setCurrentIndex(currentIndex);

        // Always generate a random probability index for UI visualization
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, ParamConstants::MAX_STEPS - 1);
        int probabilityIndex = dis(gen);
        setLastProbabilityIndex(probabilityIndex);

        // If random mode (roll) is enabled, use the probability for play/rest decisions
        if (getIsRandom()) {
            // Tick the substep first to advance the substep index
            int substepIdx = tickStepSubstepOnly(currentIndex);

            // Check play probability - if false, record rest and return -1
            if (!getPlayProbability(probabilityIndex)) {
                // Record this step as a rest (note number = -1)
                setStepNote(currentIndex, substepIdx, -1);
                return -1;
            }

            // Probability check passed - randomize the current step's note (stores unquantized)
            randomizeStepNote(currentIndex);
            // Return the quantized version for playback
            return getQuantizedStepNote(currentIndex, substepIdx);
        }

        // Not in random mode - just tick the current step normally
        return tickStep(currentIndex);
    }

    /**
     * Advance a step's substep index without returning the note
     * Used internally by tick() when we need to advance first, then decide what to do
     */
    int tickStepSubstepOnly(int stepIndex) {
        int substepLength = getStepNotesLength(stepIndex);
        int substepIndex = getStepNotesCurrentIndex(stepIndex);

        // Advance substep index
        substepIndex = (substepIndex + 1) % substepLength;
        setStepNotesCurrentIndex(stepIndex, substepIndex);

        return substepIndex;
    }

    /**
     * Tick a step's substep sequence and return the note (quantized)
     */
    int tickStep(int stepIndex) {
        int substepLength = getStepNotesLength(stepIndex);
        int substepIndex = getStepNotesCurrentIndex(stepIndex);

        // Advance substep index
        substepIndex = (substepIndex + 1) % substepLength;
        setStepNotesCurrentIndex(stepIndex, substepIndex);

        // Return quantized note for playback
        return getQuantizedStepNote(stepIndex, substepIndex);
    }

    /**
     * Randomize a step's current note within scale and range
     * NOTE: Stores UNQUANTIZED random value. Quantization happens on-the-fly during playback/display.
     * This allows changing scales without losing the underlying random data.
     */
    void randomizeStepNote(int stepIndex) {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        int rangeMin = getScaleRangeMin();
        int rangeMax = getScaleRangeMax();
        std::uniform_int_distribution<> dis(rangeMin, rangeMax);

        int randomNote = dis(gen);
        // Store the unquantized random note
        // Quantization will happen on-the-fly when reading for playback/display

        int substepIdx = getStepNotesCurrentIndex(stepIndex);
        if (substepIdx < 0) substepIdx = 0;
        setStepNote(stepIndex, substepIdx, randomNote);
    }

    /**
     * Quantize all notes in the sequence to the current scale
     */
    void quantizeAllNotesToScale() {
        for (int step = 0; step < ParamConstants::MAX_STEPS; step++) {
            for (int substep = 0; substep < ParamConstants::MAX_SUBSTEPS; substep++) {
                int note = getStepNote(step, substep);
                if (note >= 0) {  // Don't quantize rests
                    int quantized = quantizeNote(note);
                    setStepNote(step, substep, quantized);
                }
            }
        }
    }

    /**
     * Increase sequence length by 1
     */
    void increaseLength() {
        int length = getLength();
        if (length < ParamConstants::MAX_STEPS) {
            setLength(length + 1);
        }
    }

    /**
     * Decrease sequence length by 1
     */
    void decreaseLength() {
        int length = getLength();
        if (length > 1) {
            setLength(length - 1);
            // Wrap current index if needed
            int currentIndex = getCurrentIndex();
            if (currentIndex >= length - 1) {
                setCurrentIndex(length - 2);
            }
        }
    }

    /**
     * Nudge sequence left (rotate)
     */
    void nudgeLeft() {
        int length = getLength();
        if (length <= 1) return;

        // Save first step's data
        std::array<int, ParamConstants::MAX_SUBSTEPS> tempNotes;
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++) {
            tempNotes[s] = getStepNote(0, s);
        }
        int tempLength = getStepNotesLength(0);

        // Shift all steps left
        for (int i = 0; i < length - 1; i++) {
            for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++) {
                setStepNote(i, s, getStepNote(i + 1, s));
            }
            setStepNotesLength(i, getStepNotesLength(i + 1));
        }

        // Put first step at end
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++) {
            setStepNote(length - 1, s, tempNotes[s]);
        }
        setStepNotesLength(length - 1, tempLength);
    }

    /**
     * Nudge sequence right (rotate)
     */
    void nudgeRight() {
        int length = getLength();
        if (length <= 1) return;

        // Save last step's data
        std::array<int, ParamConstants::MAX_SUBSTEPS> tempNotes;
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++) {
            tempNotes[s] = getStepNote(length - 1, s);
        }
        int tempLength = getStepNotesLength(length - 1);

        // Shift all steps right
        for (int i = length - 1; i > 0; i--) {
            for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++) {
                setStepNote(i, s, getStepNote(i - 1, s));
            }
            setStepNotesLength(i, getStepNotesLength(i - 1));
        }

        // Put last step at beginning
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++) {
            setStepNote(0, s, tempNotes[s]);
        }
        setStepNotesLength(0, tempLength);
    }

    /**
     * Reset sequence to initial state
     */
    void reset() {
        setCurrentIndex(-1);
        for (int step = 0; step < ParamConstants::MAX_STEPS; step++) {
            setStepNotesCurrentIndex(step, -1);
        }
    }

    // ========================================
    // PARAMETER ACCESS HELPERS
    // ========================================

private:
    APVTS& apvts;

    int getIntParam(const juce::String& paramId) const {
        if (auto* param = apvts.getRawParameterValue(paramId)) {
            return static_cast<int>(param->load());
        }
        return 0;
    }

    void setIntParam(const juce::String& paramId, int value) {
        if (auto* param = apvts.getParameter(paramId)) {
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(value)));
        }
    }

    bool getBoolParam(const juce::String& paramId) const {
        if (auto* param = apvts.getRawParameterValue(paramId)) {
            return param->load() >= 0.5f;
        }
        return false;
    }

    void setBoolParam(const juce::String& paramId, bool value) {
        if (auto* param = apvts.getParameter(paramId)) {
            param->setValueNotifyingHost(value ? 1.0f : 0.0f);
        }
    }

    float getFloatParam(const juce::String& paramId) const {
        if (auto* param = apvts.getRawParameterValue(paramId)) {
            return param->load();
        }
        return 0.0f;
    }

    void setFloatParam(const juce::String& paramId, float value) {
        if (auto* param = apvts.getParameter(paramId)) {
            param->setValueNotifyingHost(param->convertTo0to1(value));
        }
    }
};

