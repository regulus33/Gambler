#pragma once

#include "SequenceState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

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
     * Substeps are "unrolled" — each substep becomes its own full-length step
     * in the MIDI file so the user can edit the complete melody in their DAW.
     * A step with 1 substep produces 1 MIDI step; a step with 3 substeps
     * produces 3 MIDI steps at the same tick duration each.
     */
    static juce::MidiFile createMidiFile(const SequenceState& state) {
        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote(PPQ);

        juce::MidiMessageSequence track;

        // Ticks per step based on subdivision
        int ticksPerStep = ticksForSubdivision(state.getClockSubdivision());

        // Note length as fraction of step duration
        float noteLength = state.getNoteLength();

        // MIDI channel
        int channel = getMidiChannel(state);

        int seqLength = state.getLength();
        int currentTick = 0;

        for (int step = 0; step < seqLength; ++step) {
            int substepCount = state.getStepNotesLength(step);

            // Gather velocity and CC modulations for this step
            int velocity = ParamConstants::DEFAULT_VELOCITY;
            auto mods = state.getSelectedModulations(step);

            for (int sub = 0; sub < substepCount; ++sub) {
                // CC events at the start of each unrolled step
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

                int note = state.getQuantizedStepNote(step, sub);
                if (note >= 0) {
                    int noteDurationTicks = std::max(1,
                        static_cast<int>(ticksPerStep * noteLength));

                    auto noteOn = juce::MidiMessage::noteOn(channel, note, static_cast<juce::uint8>(velocity));
                    noteOn.setTimeStamp(static_cast<double>(currentTick));
                    track.addEvent(noteOn);

                    auto noteOff = juce::MidiMessage::noteOff(channel, note);
                    noteOff.setTimeStamp(static_cast<double>(currentTick + noteDurationTicks));
                    track.addEvent(noteOff);
                }

                currentTick += ticksPerStep;
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
