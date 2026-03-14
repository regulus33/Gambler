#pragma once

#include "../data/Params.h"
#include "InternalState.h"
#include "Scale.h"
#include "ModulationType.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
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

    SequenceState(APVTS& apvts, InternalState& internalState)
        : apvts(apvts), internalState(internalState) {}
    
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
    
    // Current playback index (delegated to InternalState)
    int getCurrentIndex() const {
        return internalState.getCurrentIndex();
    }

    void setCurrentIndex(int index) {
        internalState.setCurrentIndex(juce::jlimit(-1, getLength() - 1, index));
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

    // Last probability index (delegated to InternalState)
    int getLastProbabilityIndex() const {
        return internalState.getLastProbabilityIndex();
    }

    void setLastProbabilityIndex(int index) {
        internalState.setLastProbabilityIndex(index);
    }

    // View window start (delegated to InternalState)
    int getViewWindowStart() const {
        return internalState.getViewWindowStart();
    }

    void setViewWindowStart(int startIndex) {
        internalState.setViewWindowStart(startIndex);
    }

    // ========================================
    // PLAY PROBABILITY (delegated to InternalState)
    // ========================================

    bool getPlayProbability(int index) const {
        return internalState.getPlayProbability(index);
    }

    void setPlayProbability(int index, bool shouldPlay) {
        internalState.setPlayProbability(index, shouldPlay);
    }

    void setProbabilityPercentage(int percentage) {
        percentage = juce::jlimit(0, 100, percentage);
        int numToPlay = (ParamConstants::MAX_STEPS * percentage) / 100;

        // Reset all to false
        for (int i = 0; i < ParamConstants::MAX_STEPS; i++) {
            setPlayProbability(i, false);
        }

        // Randomly set numToPlay indices to true
        // Use thread_local to avoid data races between audio and UI threads
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());

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
    // STEP NOTES (delegated to InternalState)
    // ========================================

    int getStepNote(int stepIndex, int substepIndex) const {
        return internalState.getStepNote(stepIndex, substepIndex);
    }

    void setStepNote(int stepIndex, int substepIndex, int noteNumber) {
        internalState.setStepNote(stepIndex, substepIndex, noteNumber);
    }

    int getStepNotesLength(int stepIndex) const {
        return internalState.getStepNotesLength(stepIndex);
    }

    void setStepNotesLength(int stepIndex, int length) {
        internalState.setStepNotesLength(stepIndex, length);
    }

    int getStepNotesCurrentIndex(int stepIndex) const {
        return internalState.getStepNotesCurrentIndex(stepIndex);
    }

    void setStepNotesCurrentIndex(int stepIndex, int index) {
        internalState.setStepNotesCurrentIndex(stepIndex, index);
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
     * This applies the global scale quantization on-the-fly without modifying stored data
     */
    int getQuantizedStepNote(int stepIndex, int substepIndex) const {
        int note = getStepNote(stepIndex, substepIndex);
        if (note < 0) return note;  // Don't quantize rests
        return quantizeNoteToGlobalScale(note);
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
    // STEP MODULATIONS (delegated to InternalState)
    // ========================================

    ModulationType getStepModType(int stepIndex, int modIndex) const {
        return static_cast<ModulationType>(internalState.getStepModType(stepIndex, modIndex));
    }

    void setStepModType(int stepIndex, int modIndex, ModulationType type) {
        internalState.setStepModType(stepIndex, modIndex, static_cast<int>(type));
    }

    int getStepModCCNum(int stepIndex, int modIndex) const {
        return internalState.getStepModCC(stepIndex, modIndex);
    }

    void setStepModCCNum(int stepIndex, int modIndex, int ccNum) {
        internalState.setStepModCC(stepIndex, modIndex, ccNum);
    }

    int getStepModValue(int stepIndex, int modIndex) const {
        return internalState.getStepModValue(stepIndex, modIndex);
    }

    void setStepModValue(int stepIndex, int modIndex, int value) {
        internalState.setStepModValue(stepIndex, modIndex, value);
    }

    bool getStepModSelected(int stepIndex, int modIndex) const {
        return internalState.getStepModSelected(stepIndex, modIndex);
    }

    void setStepModSelected(int stepIndex, int modIndex, bool selected) {
        internalState.setStepModSelected(stepIndex, modIndex, selected);
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
    // SCALE WINDOW (Global - applies to all steps)
    // ========================================

    // Window-based parameters (stored in APVTS)
    int getScaleWindowOffset() const {
        return getIntParam(Params::Keys::ID::SCALE_WINDOW_OFFSET);
    }

    void setScaleWindowOffset(int offset) {
        setIntParam(Params::Keys::ID::SCALE_WINDOW_OFFSET, juce::jlimit(0, 127, offset));
    }

    int getScaleWindowSize() const {
        return getIntParam(Params::Keys::ID::SCALE_WINDOW_SIZE);
    }

    void setScaleWindowSize(int size) {
        setIntParam(Params::Keys::ID::SCALE_WINDOW_SIZE, juce::jlimit(1, 128, size));
    }

    // Computed range values (for backward compatibility and convenience)
    // These compute min/max from the window offset and size
    int getScaleRangeMin() const {
        return Scale::computeMinFromWindow(getScaleWindowOffset(), getScaleWindowSize());
    }

    int getScaleRangeMax() const {
        return Scale::computeMaxFromWindow(getScaleWindowOffset(), getScaleWindowSize());
    }

    // Set range by computing window offset and size
    void setScaleRangeMin(int min) {
        int currentMax = getScaleRangeMax();
        int newOffset = Scale::computeOffsetFromRange(min, currentMax);
        int newSize = Scale::computeSizeFromRange(min, currentMax);
        setScaleWindowOffset(newOffset);
        setScaleWindowSize(newSize);
    }

    void setScaleRangeMax(int max) {
        int currentMin = getScaleRangeMin();
        int newOffset = Scale::computeOffsetFromRange(currentMin, max);
        int newSize = Scale::computeSizeFromRange(currentMin, max);
        setScaleWindowOffset(newOffset);
        setScaleWindowSize(newSize);
    }

    // ========================================
    // GLOBAL SCALE
    // ========================================

    /**
     * Get a specific note's enabled state in the global scale
     */
    bool getGlobalScaleNote(int noteIndex) const {
        if (noteIndex < 0 || noteIndex >= ParamConstants::NUM_SCALE_NOTES) return true;
        return getBoolParam(Params::Keys::ID::globalScaleNote(noteIndex));
    }

    /**
     * Set a specific note's enabled state in the global scale
     */
    void setGlobalScaleNote(int noteIndex, bool enabled) {
        if (noteIndex >= 0 && noteIndex < ParamConstants::NUM_SCALE_NOTES) {
            setBoolParam(Params::Keys::ID::globalScaleNote(noteIndex), enabled);
        }
    }

    /**
     * Get all global scale notes
     */
    std::array<int, 12> getGlobalScaleNotes() const {
        std::array<int, 12> notes;
        for (int i = 0; i < 12; i++) {
            notes[i] = getGlobalScaleNote(i) ? 1 : 0;
        }
        return notes;
    }

    /**
     * Set all global scale notes
     */
    void setGlobalScaleNotes(const std::array<int, 12>& notes) {
        for (int i = 0; i < 12; i++) {
            setGlobalScaleNote(i, notes[i] != 0);
        }
    }

    /**
     * Get a Scale object for the global scale
     */
    Scale getGlobalScale() const {
        Scale scale;
        scale.setScaleNotes(getGlobalScaleNotes());
        scale.setRangeMin(getScaleRangeMin());
        scale.setRangeMax(getScaleRangeMax());
        return scale;
    }

    /**
     * Check if a note is in the global scale
     */
    bool isNoteInGlobalScale(int noteNumber) const {
        if (noteNumber < 0 || noteNumber > 127) return false;
        int pitchClass = noteNumber % 12;
        return getGlobalScaleNote(pitchClass);
    }

    /**
     * Quantize a note to the global scale and clamp to note range.
     * Range clamping and scale quantization are applied on-the-fly without
     * modifying stored data — changing the range or scale affects playback
     * immediately.
     * Returns -1 (rest) if no scale notes are enabled.
     */
    int quantizeNoteToGlobalScale(int noteNumber) const {
        int rangeMin = getScaleRangeMin();
        int rangeMax = getScaleRangeMax();

        // Clamp to note range first
        noteNumber = juce::jlimit(rangeMin, rangeMax, noteNumber);

        // Check if any notes are enabled in the global scale
        bool anyEnabled = false;
        for (int i = 0; i < 12; i++) {
            if (getGlobalScaleNote(i)) {
                anyEnabled = true;
                break;
            }
        }
        // If no notes are enabled, return -1 (rest) to indicate "no note should play"
        if (!anyEnabled) return -1;

        // If already in scale and in range, return it
        if (isNoteInGlobalScale(noteNumber)) return noteNumber;

        // Find nearest note in scale within the range (prefer lower when equidistant)
        for (int distance = 1; distance <= 127; distance++) {
            int downNote = noteNumber - distance;
            if (downNote >= rangeMin && isNoteInGlobalScale(downNote)) return downNote;

            int upNote = noteNumber + distance;
            if (upNote <= rangeMax && isNoteInGlobalScale(upNote)) return upNote;

            // Stop searching if both directions are out of range
            if (downNote < rangeMin && upNote > rangeMax) break;
        }

        // No scale note found in range — return as rest
        return -1;
    }

    // ========================================
    // CLOCK
    // ========================================

    int getClockMode() const {
        return getIntParam(Params::Keys::ID::CLOCK_MODE);
    }

    void setClockMode(int mode) {
        setIntParam(Params::Keys::ID::CLOCK_MODE, juce::jlimit(0, 2, mode));
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

    int getMidiOutputChannel() const {
        return getIntParam(Params::Keys::ID::MIDI_OUTPUT_CHANNEL);
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

    float getNoteLength() const {
        return getFloatParam(Params::Keys::ID::PREF_NOTE_LENGTH);
    }

    void setNoteLength(float length) {
        setFloatParam(Params::Keys::ID::PREF_NOTE_LENGTH, juce::jlimit(0.0f, 1.0f, length));
    }



    // ========================================
    // NOTE COLOURS (delegated to InternalState)
    // ========================================

    /**
     * Get the colour for a chromatic note (0-11)
     * Reads from InternalState, allowing per-preset customization
     */
    juce::Colour getNoteColour(int chromaticNote) const {
        return internalState.getNoteColour(chromaticNote);
    }

    /**
     * Set the colour for a chromatic note (0-11)
     * Writes to InternalState so it's saved with presets
     */
    void setNoteColour(int chromaticNote, juce::Colour colour) {
        internalState.setNoteColour(chromaticNote, colour);
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
        // Use thread_local to avoid data races between audio and UI threads
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, ParamConstants::MAX_STEPS - 1);
        int probabilityIndex = dis(gen);
        setLastProbabilityIndex(probabilityIndex);

        // If random mode (roll) is enabled, use the probability for play/rest decisions
        if (getIsRandom()) {
            // Tick the substep first to advance the substep index
            int substepIdx = tickStepSubstepOnly(currentIndex);

            // Check play probability - if false, queue a rest and return -1
            if (!getPlayProbability(probabilityIndex)) {
                // Queue the rest note write for the message thread to apply
                queueNoteWrite(currentIndex, substepIdx, -1);
                return -1;
            }

            // Probability check passed - generate a random note
            // Queue the unquantized note for deferred storage on message thread
            // Return the quantized version for immediate playback
            int unquantizedNote = generateRandomNoteUnquantized();
            queueNoteWrite(currentIndex, substepIdx, unquantizedNote);
            return quantizeNoteToGlobalScale(unquantizedNote);
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
     * Generate a random unquantized note within the scale range.
     * Used by roll mode - the unquantized value is queued for deferred storage,
     * while quantization happens on-the-fly for playback.
     */
    int generateRandomNoteUnquantized() const {
        // Use thread_local to avoid data races between audio and UI threads
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());

        int rangeMin = getScaleRangeMin();
        int rangeMax = getScaleRangeMax();
        std::uniform_int_distribution<> dis(rangeMin, rangeMax);

        return dis(gen);
    }

    /**
     * Randomize a step's current note within scale and range
     * NOTE: Stores UNQUANTIZED random value. Quantization happens on-the-fly during playback/display.
     * This allows changing scales without losing the underlying random data.
     * WARNING: Should only be called from message thread (modifies APVTS)
     */
    void randomizeStepNote(int stepIndex) {
        // Use thread_local to avoid data races between audio and UI threads
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());

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
    // PENDING NOTE WRITES (audio thread -> message thread)
    // ========================================

    /**
     * Apply any pending note writes queued by the audio thread.
     * Must be called from the message thread (e.g., in handleAsyncUpdate).
     */
    void applyPendingNoteWrites() {
        int readIdx = pendingNoteReadIndex.load(std::memory_order_acquire);
        int writeIdx = pendingNoteWriteIndex.load(std::memory_order_acquire);

        while (readIdx != writeIdx) {
            auto& entry = pendingNoteBuffer[readIdx % PENDING_NOTE_BUFFER_SIZE];
            // Write to InternalState (step notes are now stored there, not in APVTS)
            internalState.setStepNote(entry.stepIndex, entry.substepIndex, entry.noteValue);
            readIdx++;
            pendingNoteReadIndex.store(readIdx, std::memory_order_release);
        }
    }

    // ========================================
    // PARAMETER ACCESS HELPERS
    // ========================================

private:
    APVTS& apvts;
    InternalState& internalState;

    // Lock-free SPSC ring buffer for deferring note writes from audio to message thread
    struct PendingNoteWrite {
        int stepIndex;
        int substepIndex;
        int noteValue;
    };
    static constexpr int PENDING_NOTE_BUFFER_SIZE = 64;
    std::array<PendingNoteWrite, PENDING_NOTE_BUFFER_SIZE> pendingNoteBuffer {};
    std::atomic<int> pendingNoteWriteIndex { 0 };
    std::atomic<int> pendingNoteReadIndex { 0 };

    void queueNoteWrite(int stepIndex, int substepIndex, int noteValue) {
        int writeIdx = pendingNoteWriteIndex.load(std::memory_order_relaxed);
        int readIdx = pendingNoteReadIndex.load(std::memory_order_acquire);
        // Drop if buffer full (shouldn't happen at normal tick rates)
        if (writeIdx - readIdx >= PENDING_NOTE_BUFFER_SIZE) return;
        auto& entry = pendingNoteBuffer[writeIdx % PENDING_NOTE_BUFFER_SIZE];
        entry.stepIndex = stepIndex;
        entry.substepIndex = substepIndex;
        entry.noteValue = noteValue;
        pendingNoteWriteIndex.store(writeIdx + 1, std::memory_order_release);
    }

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

