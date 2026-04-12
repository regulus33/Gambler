#pragma once

#include "InternalState.h"
#include "ModulationType.h"
#include "../data/Params.h"

/**
 * StepModulationData - Lightweight struct for step modulation data
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
 * Step - A lightweight view into one step of the sequencer.
 *
 * Holds a reference to InternalState + a step index. No owned data,
 * no allocation — just an ergonomic OOP interface over the flat backing store.
 *
 * Usage:
 *   Step s(internalState, 3);
 *   int note = s.note(0);        // substep 0's raw note
 *   s.setNote(0, 72);            // set substep 0 to C5
 *   int cur = s.effectiveCurrentSubstep(); // which substep is active
 *   int played = s.tick();       // advance substep index, return raw note
 */
class Step {
public:
    Step(InternalState& s, int stepIndex)
        : state(s), index(stepIndex) {}

    int getIndex() const { return index; }

    // ========================================
    // SUBSTEP NOTES
    // ========================================

    int note(int substep) const {
        return state.getStepNote(index, substep);
    }

    void setNote(int substep, int noteNumber) {
        state.setStepNote(index, substep, noteNumber);
    }

    int substepCount() const {
        return state.getStepNotesLength(index);
    }

    void setSubstepCount(int length) {
        state.setStepNotesLength(index, length);
    }

    int currentSubstep() const {
        return state.getStepNotesCurrentIndex(index);
    }

    void setCurrentSubstep(int idx) {
        state.setStepNotesCurrentIndex(index, idx);
    }

    int selectedSubstep() const {
        return state.getSelectedSubstepIndex(index);
    }

    void setSelectedSubstep(int idx) {
        state.setSelectedSubstepIndex(index, idx);
    }

    /** The substep that is currently active (clamped so pre-start -1 maps to 0). */
    int effectiveCurrentSubstep() const {
        int cs = currentSubstep();
        return (cs < 0) ? 0 : cs;
    }

    /** Raw (unquantized) note for the currently-active substep. */
    int rawCurrentNote() const {
        return note(effectiveCurrentSubstep());
    }

    /** True if the currently-active substep is a rest (-1). */
    bool isRest() const {
        return rawCurrentNote() < 0;
    }

    // ========================================
    // SUBSTEP ADVANCEMENT
    // ========================================

    /**
     * Advance the substep index and return the raw note at the new position.
     * This is the per-step tick used during playback.
     */
    int tick() {
        int len = substepCount();
        int idx = (currentSubstep() + 1) % len;
        setCurrentSubstep(idx);
        return note(idx);
    }

    /**
     * Advance the substep index without returning the note.
     * Returns the new substep index.
     */
    int advanceSubstep() {
        int len = substepCount();
        int idx = (currentSubstep() + 1) % len;
        setCurrentSubstep(idx);
        return idx;
    }

    // ========================================
    // MODULATIONS
    // ========================================

    ModulationType modType(int modIndex) const {
        return static_cast<ModulationType>(state.getStepModType(index, modIndex));
    }

    void setModType(int modIndex, ModulationType type) {
        state.setStepModType(index, modIndex, static_cast<int>(type));
    }

    int modCCNumber(int modIndex) const {
        return state.getStepModCC(index, modIndex);
    }

    void setModCCNumber(int modIndex, int ccNum) {
        state.setStepModCC(index, modIndex, ccNum);
    }

    int modValue(int modIndex) const {
        return state.getStepModValue(index, modIndex);
    }

    void setModValue(int modIndex, int value) {
        state.setStepModValue(index, modIndex, value);
    }

    bool modSelected(int modIndex) const {
        return state.getStepModSelected(index, modIndex);
    }

    void setModSelected(int modIndex, bool selected) {
        state.setStepModSelected(index, modIndex, selected);
    }

    std::vector<StepModulationData> selectedModulations() const {
        std::vector<StepModulationData> result;
        for (int i = 0; i < ParamConstants::NUM_MODULATIONS; i++) {
            if (modSelected(i)) {
                StepModulationData mod;
                mod.type = modType(i);
                mod.ccNumber = modCCNumber(i);
                mod.value = modValue(i);
                mod.selected = true;
                result.push_back(mod);
            }
        }
        return result;
    }

    // ========================================
    // BULK NOTE OPERATIONS
    // ========================================

    /** Copy all substep notes and length from another step. */
    void copyNotesFrom(const Step& other) {
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++) {
            setNote(s, other.note(s));
        }
        setSubstepCount(other.substepCount());
    }

private:
    InternalState& state;
    int index;
};
