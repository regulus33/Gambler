#pragma once

#include "Step.h"
#include "ScaleQuantizer.h"
#include "InternalState.h"
#include "../algorithms/probability/EdenGrowthPattern.h"
#include "../data/Params.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <random>
#include <vector>

/**
 * Sequence - The sequencer's playback and mutation engine.
 *
 * Owns tick (advance + play), nudge, randomize, reset, and the
 * pending-note-write buffer for audio→message thread handoff.
 * Reads length/isRandom from APVTS, step data from InternalState,
 * and quantization from ScaleQuantizer.
 *
 * Usage:
 *   Sequence seq(apvts, internal, scaleQ);
 *   int note = seq.tick();           // advance playhead, return quantized note
 *   seq.step(3).setNote(0, 72);      // direct step access
 *   seq.nudgeLeft();                  // rotate steps
 */
class Sequence {
public:
    using APVTS = juce::AudioProcessorValueTreeState;

    Sequence(APVTS& apvts, InternalState& internal, ScaleQuantizer& scaleQ)
        : apvts(apvts), internal(internal), scaleQ(scaleQ)
    {
        probabilityPattern.setGridDim(dimForLevel(internal.getProbabilityDivisionLevel()));
    }

    // ========================================
    // STEP ACCESS
    // ========================================

    Step step(int i) { return Step(internal, i); }
    const Step step(int i) const { return Step(const_cast<InternalState&>(internal), i); }

    Step operator[](int i) { return step(i); }

    // ========================================
    // LENGTH
    // ========================================

    int length() const {
        return getInt(Params::Keys::ID::SEQ_LENGTH);
    }

    void setLength(int len) {
        setInt(Params::Keys::ID::SEQ_LENGTH,
            juce::jlimit(1, ParamConstants::MAX_STEPS, len));
    }

    void increaseLength() {
        if (length() < ParamConstants::MAX_STEPS)
            setLength(length() + 1);
    }

    void decreaseLength() {
        int len = length();
        if (len > 1) {
            setLength(len - 1);
            if (currentIndex() >= len - 1)
                setCurrentIndex(len - 2);
        }
    }

    // ========================================
    // PLAYBACK INDEX
    // ========================================

    int currentIndex() const {
        return internal.getCurrentIndex();
    }

    void setCurrentIndex(int index) {
        internal.setCurrentIndex(juce::jlimit(-1, length() - 1, index));
    }

    // ========================================
    // PROBABILITY
    // ========================================

    int lastProbabilityIndex() const {
        return internal.getLastProbabilityIndex();
    }

    void setLastProbabilityIndex(int index) {
        internal.setLastProbabilityIndex(index);
    }

    bool playProbability(int index) const {
        return internal.getPlayProbability(index);
    }

    void setPlayProbability(int index, bool shouldPlay) {
        internal.setPlayProbability(index, shouldPlay);
    }

    void setProbabilityPercentage(int percentage) {
        currentProbabilityPercentage = juce::jlimit(0, 100, percentage);

        const int dim = probabilityPattern.gridDim();
        const int gridCells = dim * dim;
        const int numToPlay = roundToNearest(currentProbabilityPercentage, gridCells);

        probabilityPattern.setActiveCount(numToPlay);
        writePatternToProbabilitySteps(probabilityPattern.cells());
    }

    // Re-roll the Eden cluster at the current percentage from a fresh seed.
    void reseedProbabilityPattern() {
        thread_local std::random_device rd;
        probabilityPattern.reseed(rd());
        setProbabilityPercentage(currentProbabilityPercentage);
    }

    // Switch the probability gauge between 8x8, 4x4, and 2x2.
    // Preserves percentage by re-applying it on the new grid.
    void setProbabilityDivisionLevel(int level) {
        level = juce::jlimit(0, 2, level);
        internal.setProbabilityDivisionLevel(level);
        probabilityPattern.setGridDim(dimForLevel(level));
        setProbabilityPercentage(currentProbabilityPercentage);
    }

    int probabilityDivisionLevel() const {
        return internal.getProbabilityDivisionLevel();
    }

    int probabilityGridDim() const {
        return probabilityPattern.gridDim();
    }

    // Uniform random scatter at the current percentage and division.
    // Bypasses Eden — useful when the user wants a less clumped pattern.
    void randomizeProbabilityUniform() {
        const int dim = probabilityPattern.gridDim();
        const int gridCells = dim * dim;
        const int numToPlay = roundToNearest(currentProbabilityPercentage, gridCells);

        std::array<bool, algorithms::probability::Grid::maxCellCount> picks{};

        std::vector<int> indices;
        indices.reserve(static_cast<std::size_t>(gridCells));
        for (int i = 0; i < gridCells; ++i) indices.push_back(i);

        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());
        std::shuffle(indices.begin(), indices.end(), gen);

        for (int i = 0; i < numToPlay; ++i)
            picks[static_cast<std::size_t>(indices[i])] = true;

        writePatternToProbabilitySteps(picks);
    }

    // ========================================
    // RANDOM MODE
    // ========================================

    bool isRandom() const {
        return getBool(Params::Keys::ID::SEQ_IS_RANDOM);
    }

    // ========================================
    // TICK (main playback advance)
    // ========================================

    /**
     * Advance the sequencer to the next step and return the note to play.
     * Called from processBlock.
     * @return quantized MIDI note, or -1 for rest
     */
    int tick() {
        int len = length();
        int idx = (currentIndex() + 1) % len;
        setCurrentIndex(idx);

        const int displayCells = probabilityPattern.gridCellCount();
        int tickN = (lastProbabilityIndex() + 1) % displayCells;
        setLastProbabilityIndex(tickN);
        const int probIdx = tickToUnderlyingIndex(tickN);

        if (isRandom()) {
            int substepIdx = step(idx).advanceSubstep();

            if (!playProbability(probIdx)) {
                queueNoteWrite(idx, substepIdx, -1);
                return -1;
            }

            int unquantized = generateRandomNoteUnquantized();
            int baked = scaleQ.bake(unquantized);
            queueNoteWrite(idx, substepIdx, baked);
            return scaleQ.destructiveScale() ? baked : scaleQ.quantize(unquantized);
        }

        return tickStep(idx);
    }

    /** Tick a single step: advance its substep and return the quantized note. */
    int tickStep(int stepIndex) {
        Step s = step(stepIndex);
        int substepIndex = s.advanceSubstep();
        int note = s.note(substepIndex);
        if (note < 0) return note;
        return scaleQ.quantize(note);
    }

    // ========================================
    // NUDGE (rotate steps)
    // ========================================

    void nudgeLeft() {
        int len = length();
        if (len <= 1) return;

        Step first = step(0);
        std::array<int, ParamConstants::MAX_SUBSTEPS> tempNotes;
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++)
            tempNotes[s] = first.note(s);
        int tempLen = first.substepCount();

        for (int i = 0; i < len - 1; i++)
            step(i).copyNotesFrom(step(i + 1));

        Step last = step(len - 1);
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++)
            last.setNote(s, tempNotes[s]);
        last.setSubstepCount(tempLen);
    }

    void nudgeRight() {
        int len = length();
        if (len <= 1) return;

        Step last = step(len - 1);
        std::array<int, ParamConstants::MAX_SUBSTEPS> tempNotes;
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++)
            tempNotes[s] = last.note(s);
        int tempLen = last.substepCount();

        for (int i = len - 1; i > 0; i--)
            step(i).copyNotesFrom(step(i - 1));

        Step first = step(0);
        for (int s = 0; s < ParamConstants::MAX_SUBSTEPS; s++)
            first.setNote(s, tempNotes[s]);
        first.setSubstepCount(tempLen);
    }

    // ========================================
    // RANDOMIZE
    // ========================================

    int generateRandomNoteUnquantized() const {
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());

        int rMin = scaleQ.rangeMin();
        int rMax = scaleQ.rangeMax();
        std::uniform_int_distribution<> dis(rMin, rMax);
        return dis(gen);
    }

    void randomizeStepNote(int stepIndex) {
        int randomNote = scaleQ.bake(generateRandomNoteUnquantized());

        Step s = step(stepIndex);
        int substepIdx = s.currentSubstep();
        if (substepIdx < 0) substepIdx = 0;
        s.setNote(substepIdx, randomNote);
    }

    /** Bake all existing notes through the current scale/range. */
    void bakeAllExistingNotes() {
        for (int i = 0; i < ParamConstants::MAX_STEPS; ++i) {
            Step s = step(i);
            for (int sub = 0; sub < ParamConstants::MAX_SUBSTEPS; ++sub) {
                int note = s.note(sub);
                int baked = scaleQ.bake(note);
                if (baked != note)
                    s.setNote(sub, baked);
            }
        }
    }

    // ========================================
    // RESET
    // ========================================

    void reset() {
        setCurrentIndex(-1);
        for (int i = 0; i < ParamConstants::MAX_STEPS; i++)
            step(i).setCurrentSubstep(-1);
    }

    // ========================================
    // PENDING NOTE WRITES (audio → message thread)
    // ========================================

    void applyPendingNoteWrites() {
        int readIdx = pendingNoteReadIndex.load(std::memory_order_acquire);
        int writeIdx = pendingNoteWriteIndex.load(std::memory_order_acquire);

        while (readIdx != writeIdx) {
            auto& entry = pendingNoteBuffer[readIdx % PENDING_NOTE_BUFFER_SIZE];
            internal.setStepNote(entry.stepIndex, entry.substepIndex, entry.noteValue);
            readIdx++;
            pendingNoteReadIndex.store(readIdx, std::memory_order_release);
        }
    }

private:
    APVTS& apvts;
    InternalState& internal;
    ScaleQuantizer& scaleQ;

    algorithms::probability::EdenGrowthPattern probabilityPattern;
    int currentProbabilityPercentage { 100 };

    static int dimForLevel(int level) noexcept {
        // Levels 0/1/2 → grids 8x8 / 4x4 / 2x2.
        switch (level) {
            case 1: return 4;
            case 2: return 2;
            default: return 8;
        }
    }

    static int roundToNearest(int percent, int totalCells) noexcept {
        return (percent * totalCells + 50) / 100;
    }

    // Map a 0..(D*D - 1) tick counter to the top-left underlying step of the
    // corresponding display block, walking display cells in row-major order.
    // One tick advances the highlight by exactly one display cell.
    int tickToUnderlyingIndex(int tickN) const {
        const int D = probabilityPattern.gridDim();
        const int B = 8 / D;
        const int dRow = tickN / D;
        const int dCol = tickN % D;
        return (dRow * B) * 8 + (dCol * B);
    }

    // Tile the pattern's DxD output across the 64-step probability array so
    // each pattern cell controls an (8/D)x(8/D) block of underlying steps.
    void writePatternToProbabilitySteps(
        const algorithms::probability::EdenGrowthPattern::Cells& patternCells)
    {
        const int dim = probabilityPattern.gridDim();
        const int blockSize = 8 / dim;

        for (int patternRow = 0; patternRow < dim; ++patternRow) {
            for (int patternCol = 0; patternCol < dim; ++patternCol) {
                const bool on = patternCells[
                    static_cast<std::size_t>(patternRow * dim + patternCol)];
                for (int dr = 0; dr < blockSize; ++dr) {
                    for (int dc = 0; dc < blockSize; ++dc) {
                        const int row = patternRow * blockSize + dr;
                        const int col = patternCol * blockSize + dc;
                        setPlayProbability(row * 8 + col, on);
                    }
                }
            }
        }
    }

    // Lock-free SPSC ring buffer
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
        int wIdx = pendingNoteWriteIndex.load(std::memory_order_relaxed);
        int rIdx = pendingNoteReadIndex.load(std::memory_order_acquire);
        if (wIdx - rIdx >= PENDING_NOTE_BUFFER_SIZE) return;
        auto& entry = pendingNoteBuffer[wIdx % PENDING_NOTE_BUFFER_SIZE];
        entry.stepIndex = stepIndex;
        entry.substepIndex = substepIndex;
        entry.noteValue = noteValue;
        pendingNoteWriteIndex.store(wIdx + 1, std::memory_order_release);
    }

    int getInt(const juce::String& id) const {
        if (auto* p = apvts.getRawParameterValue(id))
            return static_cast<int>(p->load());
        return 0;
    }

    void setInt(const juce::String& id, int value) {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(value)));
    }

    bool getBool(const juce::String& id) const {
        if (auto* p = apvts.getRawParameterValue(id))
            return p->load() >= 0.5f;
        return false;
    }
};
