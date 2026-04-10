#pragma once

#include "../data/Params.h"
#include "InternalState.h"
#include "Step.h"
#include "Scale.h"
#include "ScaleQuantizer.h"
#include "Sequence.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <vector>

/**
 * SequenceState - APVTS-backed sequencer state accessor
 *
 * Provides a domain API over APVTS (automatable params) and InternalState
 * (per-step data). Step-level operations are delegated to Step view objects.
 *
 * Usage:
 *   SequenceState state(apvts, internal);
 *   Step s = state.step(0);          // get a Step view
 *   int note = s.note(0);            // read substep 0
 *   state.setStepNote(0, 0, 72);     // legacy API still works
 */
class SequenceState {
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    SequenceState(APVTS& apvts, InternalState& internalState)
        : apvts(apvts), internalState(internalState), scaleQ(apvts), seq(apvts, internalState, scaleQ) {}

    /** Access the domain objects directly. */
    Sequence& sequence() { return seq; }
    const Sequence& sequence() const { return seq; }
    ScaleQuantizer& scaleQuantizer() { return scaleQ; }
    const ScaleQuantizer& scaleQuantizer() const { return scaleQ; }

    /** Get a Step view for the given index. Lightweight — no allocation. */
    Step step(int stepIndex) { return seq.step(stepIndex); }
    const Step step(int stepIndex) const { return seq.step(stepIndex); }

    // ========================================
    // SEQUENCE METADATA (delegated through Sequence)
    // ========================================

    int getLength() const { return seq.length(); }
    void setLength(int length) { seq.setLength(length); }

    int getCurrentIndex() const { return seq.currentIndex(); }
    void setCurrentIndex(int index) { seq.setCurrentIndex(index); }

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

    int getLastProbabilityIndex() const { return seq.lastProbabilityIndex(); }
    void setLastProbabilityIndex(int index) { seq.setLastProbabilityIndex(index); }

    int getViewWindowStart() const { return internalState.getViewWindowStart(); }
    void setViewWindowStart(int startIndex) { internalState.setViewWindowStart(startIndex); }

    // ========================================
    // PLAY PROBABILITY (delegated through Sequence)
    // ========================================

    bool getPlayProbability(int index) const { return seq.playProbability(index); }
    void setPlayProbability(int index, bool shouldPlay) { seq.setPlayProbability(index, shouldPlay); }
    void setProbabilityPercentage(int percentage) { seq.setProbabilityPercentage(percentage); }
    
    // ========================================
    // STEP NOTES (delegated through Step)
    // ========================================

    int getStepNote(int stepIndex, int substepIndex) const {
        return step(stepIndex).note(substepIndex);
    }

    void setStepNote(int stepIndex, int substepIndex, int noteNumber) {
        step(stepIndex).setNote(substepIndex, noteNumber);
    }

    int getStepNotesLength(int stepIndex) const {
        return step(stepIndex).substepCount();
    }

    void setStepNotesLength(int stepIndex, int length) {
        step(stepIndex).setSubstepCount(length);
    }

    int getStepNotesCurrentIndex(int stepIndex) const {
        return step(stepIndex).currentSubstep();
    }

    int getNextSubstepIndex(int stepIndex) const {
        return step(stepIndex).nextSubstep();
    }

    void setStepNotesCurrentIndex(int stepIndex, int index) {
        step(stepIndex).setCurrentSubstep(index);
    }

    int getSelectedSubstepIndex(int stepIndex) const {
        return step(stepIndex).selectedSubstep();
    }

    void setSelectedSubstepIndex(int stepIndex, int index) {
        step(stepIndex).setSelectedSubstep(index);
    }

    int getRawCurrentNoteForStep(int stepIndex) const {
        return step(stepIndex).rawCurrentNote();
    }

    /** Quantize a step's note on-the-fly without modifying stored data. */
    int getQuantizedStepNote(int stepIndex, int substepIndex) const {
        int note = step(stepIndex).note(substepIndex);
        if (note < 0) return note;
        return quantizeNoteToGlobalScale(note);
    }

    int getQuantizedCurrentNoteForStep(int stepIndex) const {
        return getQuantizedStepNote(stepIndex, step(stepIndex).nextSubstep());
    }

    /** Main accessor for UI — returns quantized note for the next-to-play substep. */
    int getCurrentNoteForStep(int stepIndex) const {
        return getQuantizedCurrentNoteForStep(stepIndex);
    }

    void updateCurrentNoteForStep(int stepIndex, int noteNumber) {
        step(stepIndex).setNote(step(stepIndex).nextSubstep(), bakeNoteForStorage(noteNumber));
    }

    int getSelectedNoteForStep(int stepIndex) const {
        int substepIdx = step(stepIndex).selectedSubstep();
        if (substepIdx < 0) substepIdx = 0;
        return getQuantizedStepNote(stepIndex, substepIdx);
    }

    void updateSelectedNoteForStep(int stepIndex, int noteNumber) {
        int substepIdx = step(stepIndex).selectedSubstep();
        if (substepIdx < 0) substepIdx = 0;
        step(stepIndex).setNote(substepIdx, bakeNoteForStorage(noteNumber));
    }

    bool stepHasRest(int stepIndex) const {
        return step(stepIndex).isRest();
    }

    // ========================================
    // STEP MODULATIONS (delegated through Step)
    // ========================================

    ModulationType getStepModType(int stepIndex, int modIndex) const {
        return step(stepIndex).modType(modIndex);
    }

    void setStepModType(int stepIndex, int modIndex, ModulationType type) {
        step(stepIndex).setModType(modIndex, type);
    }

    int getStepModCCNum(int stepIndex, int modIndex) const {
        return step(stepIndex).modCCNumber(modIndex);
    }

    void setStepModCCNum(int stepIndex, int modIndex, int ccNum) {
        step(stepIndex).setModCCNumber(modIndex, ccNum);
    }

    int getStepModValue(int stepIndex, int modIndex) const {
        return step(stepIndex).modValue(modIndex);
    }

    void setStepModValue(int stepIndex, int modIndex, int value) {
        step(stepIndex).setModValue(modIndex, value);
    }

    bool getStepModSelected(int stepIndex, int modIndex) const {
        return step(stepIndex).modSelected(modIndex);
    }

    void setStepModSelected(int stepIndex, int modIndex, bool selected) {
        step(stepIndex).setModSelected(modIndex, selected);
    }

    std::vector<StepModulationData> getSelectedModulations(int stepIndex) const {
        return step(stepIndex).selectedModulations();
    }

    // ========================================
    // SCALE / RANGE / QUANTIZATION (delegated through ScaleQuantizer)
    // ========================================

    int getScaleWindowOffset() const { return scaleQ.windowOffset(); }
    void setScaleWindowOffset(int offset) { scaleQ.setWindowOffset(offset); }
    int getScaleWindowSize() const { return scaleQ.windowSize(); }
    void setScaleWindowSize(int size) { scaleQ.setWindowSize(size); }

    int getScaleRangeMin() const { return scaleQ.rangeMin(); }
    int getScaleRangeMax() const { return scaleQ.rangeMax(); }
    void setScaleRangeMin(int min) { scaleQ.setRangeMin(min); }
    void setScaleRangeMax(int max) { scaleQ.setRangeMax(max); }

    bool getGlobalScaleNote(int noteIndex) const { return scaleQ.scaleNote(noteIndex); }
    void setGlobalScaleNote(int noteIndex, bool enabled) { scaleQ.setScaleNote(noteIndex, enabled); }
    std::array<int, 12> getGlobalScaleNotes() const { return scaleQ.scaleNotes(); }
    void setGlobalScaleNotes(const std::array<int, 12>& notes) { scaleQ.setScaleNotes(notes); }
    Scale getGlobalScale() const { return scaleQ.asScale(); }
    bool isNoteInGlobalScale(int noteNumber) const { return scaleQ.isInScale(noteNumber); }

    int quantizeNoteToGlobalScale(int noteNumber) const { return scaleQ.quantize(noteNumber); }
    int bakeNoteForStorage(int noteNumber) const { return scaleQ.bake(noteNumber); }

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

    bool getDestructiveScale() const { return scaleQ.destructiveScale(); }
    void setDestructiveScale(bool enabled) { scaleQ.setDestructiveScale(enabled); }
    bool getDestructiveRange() const { return scaleQ.destructiveRange(); }
    void setDestructiveRange(bool enabled) { scaleQ.setDestructiveRange(enabled); }

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
    // SEQUENCER OPERATIONS (delegated through Sequence)
    // ========================================

    int tick() { return seq.tick(); }
    int tickStep(int stepIndex) { return seq.tickStep(stepIndex); }
    int generateRandomNoteUnquantized() const { return seq.generateRandomNoteUnquantized(); }
    void bakeAllExistingNotes() { seq.bakeAllExistingNotes(); }
    void randomizeStepNote(int stepIndex) { seq.randomizeStepNote(stepIndex); }
    void increaseLength() { seq.increaseLength(); }
    void decreaseLength() { seq.decreaseLength(); }
    void nudgeLeft() { seq.nudgeLeft(); }
    void nudgeRight() { seq.nudgeRight(); }
    void reset() { seq.reset(); }
    void applyPendingNoteWrites() { seq.applyPendingNoteWrites(); }

    // ========================================
    // PARAMETER ACCESS HELPERS
    // ========================================

private:
    APVTS& apvts;
    InternalState& internalState;
    ScaleQuantizer scaleQ;
    Sequence seq;

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

