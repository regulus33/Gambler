#pragma once

#include "SequenceState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <numeric>
#include <vector>

/**
 * MidiExporter - Converts current sequence state to a standard MIDI file.
 *
 * Reads step notes, velocities, CC modulations, tempo, and subdivision
 * from SequenceState and produces a Type 0 MIDI file at 480 PPQ.
 */
class MidiExporter {
public:
    static constexpr int PPQ = 480;

    /**
     * Build a MidiFile from the current SequenceState.
     *
     * The exported file is a faithful render of one natural-loop's worth of
     * the live sequencer's output: the same algorithm Sequence::tick() would
     * produce, walked for LCM(substepCounts) full passes. With all substep
     * counts equal to 1, that is one pass (the common case); when any step
     * has a substep count > 1, the file unrolls just enough passes that
     * every substep cursor is simultaneously back at 0 — i.e. the file
     * loops cleanly in a DAW and stays in lock-step with what live
     * playback does forever.
     */
    static juce::MidiFile createMidiFile(const SequenceState& state) {
        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote(PPQ);

        juce::MidiMessageSequence track;

        const int ticksPerStep = ticksForSubdivision(state.getClockSubdivision());
        const float noteLength = state.getNoteLength();
        const int channel      = getMidiChannel(state);
        const int seqLength    = state.getLength();
        if (seqLength <= 0) {
            midiFile.addTrack(track);
            return midiFile;
        }

        // Number of full passes we need: LCM of every step's substep count.
        // Steps with substep count 0 are treated as 1 (defensive — should
        // never happen in practice).
        long long passes = 1;
        for (int s = 0; s < seqLength; ++s) {
            const long long count = std::max(1, state.getStepNotesLength(s));
            passes = std::lcm(passes, count);
        }

        // Per-step substep cursor. Mirrors Step::advanceSubstep but read-
        // then-advance (Sequence::reset() leaves the live cursor at -1 so
        // the first pre-incrementing call returns 0 — equivalent).
        std::vector<int> cursors(static_cast<size_t>(seqLength), 0);

        int currentTick = 0;
        for (long long pass = 0; pass < passes; ++pass) {
            for (int step = 0; step < seqLength; ++step) {
                const int substepCount = std::max(1, state.getStepNotesLength(step));
                const int sub          = cursors[(size_t) step];

                int velocity = ParamConstants::DEFAULT_VELOCITY;
                auto mods    = state.getSelectedModulations(step);
                for (const auto& mod : mods) {
                    if (mod.getType() == ModulationType::Velocity) {
                        velocity = mod.getValue();
                    } else if (mod.getType() == ModulationType::CC) {
                        auto ccMsg = juce::MidiMessage::controllerEvent(
                            channel, mod.getCCNumber(), mod.getValue());
                        ccMsg.setTimeStamp(static_cast<double>(currentTick));
                        track.addEvent(ccMsg);
                    }
                }

                const int note = state.getQuantizedStepNote(step, sub);
                if (note >= 0) {
                    const int noteDurationTicks = std::max(1,
                        static_cast<int>(static_cast<float>(ticksPerStep) * noteLength));

                    auto noteOn = juce::MidiMessage::noteOn(channel, note,
                        static_cast<juce::uint8>(velocity));
                    noteOn.setTimeStamp(static_cast<double>(currentTick));
                    track.addEvent(noteOn);

                    auto noteOff = juce::MidiMessage::noteOff(channel, note);
                    noteOff.setTimeStamp(static_cast<double>(currentTick + noteDurationTicks));
                    track.addEvent(noteOff);
                }

                currentTick += ticksPerStep;
                cursors[(size_t) step] = (sub + 1) % substepCount;
            }
        }

        track.sort();
        track.updateMatchedPairs();
        midiFile.addTrack(track);
        return midiFile;
    }

    /**
     * Write a MidiFile to disk.
     * @return true on success.
     */
    static bool writeToFile(const juce::MidiFile& midiFile, const juce::File& file) {
        file.deleteFile();
        juce::FileOutputStream stream(file);
        if (!stream.openedOk()) return false;
        return midiFile.writeTo(stream);
    }

    /**
     * Write to a temp file and return the path (useful for drag-and-drop).
     */
    static juce::File writeToTempFile(const juce::MidiFile& midiFile) {
        auto tempDir = juce::File::getSpecialLocation(
            juce::File::tempDirectory);
        auto tempFile = tempDir.getChildFile("Gambler-Export.mid");
        writeToFile(midiFile, tempFile);
        return tempFile;
    }

private:
    static int ticksForSubdivision(int subdivision) {
        // 0=quarter, 1=eighth, 2=sixteenth, 3=thirty-second
        switch (subdivision) {
            case 0: return PPQ;        // quarter = 480
            case 1: return PPQ / 2;    // eighth  = 240
            case 2: return PPQ / 4;    // 16th    = 120
            case 3: return PPQ / 8;    // 32nd    = 60
            default: return PPQ / 4;
        }
    }

    static int getMidiChannel(const SequenceState& state) {
        // APVTS stores 1-16, MidiMessage expects 1-16
        return state.getMidiOutputChannel();
    }
};
