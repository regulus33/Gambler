#pragma once

#include "backend/Sequence.h"
#include <vector>
#include <string>

/**
 * SequenceBuilder - Builder pattern for creating test sequences
 * 
 * Makes tests more readable by providing a fluent interface for
 * constructing complex sequence configurations.
 */
class SequenceBuilder {
public:
    SequenceBuilder() = default;
    
    SequenceBuilder& withLength(int len) {
        sequence.setLength(len);
        return *this;
    }
    
    SequenceBuilder& withStep(int index, std::vector<int> notes) {
        auto& step = sequence.getStep(index);
        step.getStepNotes().setLength(static_cast<int>(notes.size()));
        for (size_t i = 0; i < notes.size(); i++) {
            step.getStepNotes().setNote(static_cast<int>(i), notes[i]);
        }
        return *this;
    }
    
    SequenceBuilder& withStepNote(int stepIndex, int noteIndex, int noteNumber) {
        sequence.getStep(stepIndex).getStepNotes().setNote(noteIndex, noteNumber);
        return *this;
    }
    
    SequenceBuilder& withStepLength(int stepIndex, int length) {
        sequence.getStep(stepIndex).getStepNotes().setLength(length);
        return *this;
    }
    
    SequenceBuilder& withScale(const std::vector<std::string>& noteNames) {
        sequence.getScale().setScaleFromNames(noteNames);
        return *this;
    }
    
    SequenceBuilder& withRange(int min, int max) {
        sequence.getScale().setRange(min, max);
        return *this;
    }
    
    SequenceBuilder& withRandomEnabled(bool enabled) {
        sequence.setIsRandom(enabled);
        return *this;
    }
    
    SequenceBuilder& withProbability(int percentage) {
        sequence.setProbabilityPercentage(percentage);
        return *this;
    }
    
    SequenceBuilder& withPlayProbability(int index, bool shouldPlay) {
        sequence.setPlayProbability(index, shouldPlay);
        return *this;
    }
    
    Sequence build() {
        return sequence;
    }
    
    Sequence& get() {
        return sequence;
    }
    
private:
    Sequence sequence;
};

/**
 * Helper function to tick a sequence N times and collect the notes
 * 
 * @param seq Sequence to tick
 * @param count Number of times to tick
 * @return Vector of note numbers (includes -1 for rests)
 */
inline std::vector<int> tickAndCollect(Sequence& seq, int count) {
    std::vector<int> notes;
    for (int i = 0; i < count; i++) {
        notes.push_back(seq.tick());
    }
    return notes;
}

/**
 * Helper function to check if a sequence produces expected pattern
 * 
 * @param seq Sequence to test
 * @param expected Expected note pattern
 * @return true if pattern matches
 */
inline bool hasNotePattern(Sequence& seq, const std::vector<int>& expected) {
    auto actual = tickAndCollect(seq, static_cast<int>(expected.size()));
    return actual == expected;
}

