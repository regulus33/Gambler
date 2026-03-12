#include <catch2/catch_test_macros.hpp>
#include "../source/PluginProcessor.h"
#include "../source/backend/MidiExporter.h"

/**
 * MidiExporter Tests
 *
 * Verifies MIDI file export produces correct output for various
 * sequence configurations including substep unrolling, note range,
 * scale quantization, and variable-length sequences.
 */

class ExporterFixture {
public:
    ExporterFixture() {}

    PluginProcessor& getProcessor() { return processor; }
    SequenceState& getState() { return processor.getState(); }

    juce::MidiFile exportMidi() {
        return MidiExporter::createMidiFile(getState());
    }

    // Count note-on events in the exported MIDI file
    int countNoteOns(const juce::MidiFile& midiFile) {
        int count = 0;
        for (int t = 0; t < midiFile.getNumTracks(); ++t) {
            auto* track = midiFile.getTrack(t);
            for (int i = 0; i < track->getNumEvents(); ++i) {
                if (track->getEventPointer(i)->message.isNoteOn())
                    ++count;
            }
        }
        return count;
    }

    // Get the last event timestamp (clip length in ticks)
    double getLastTimestamp(const juce::MidiFile& midiFile) {
        double last = 0.0;
        for (int t = 0; t < midiFile.getNumTracks(); ++t) {
            auto* track = midiFile.getTrack(t);
            for (int i = 0; i < track->getNumEvents(); ++i) {
                double ts = track->getEventPointer(i)->message.getTimeStamp();
                if (ts > last) last = ts;
            }
        }
        return last;
    }

    // Collect all note-on MIDI note numbers in order
    std::vector<int> getNoteOns(const juce::MidiFile& midiFile) {
        std::vector<int> notes;
        for (int t = 0; t < midiFile.getNumTracks(); ++t) {
            auto* track = midiFile.getTrack(t);
            for (int i = 0; i < track->getNumEvents(); ++i) {
                auto& msg = track->getEventPointer(i)->message;
                if (msg.isNoteOn())
                    notes.push_back(msg.getNoteNumber());
            }
        }
        return notes;
    }

    // Collect all note-on timestamps in order
    std::vector<double> getNoteOnTimestamps(const juce::MidiFile& midiFile) {
        std::vector<double> timestamps;
        for (int t = 0; t < midiFile.getNumTracks(); ++t) {
            auto* track = midiFile.getTrack(t);
            for (int i = 0; i < track->getNumEvents(); ++i) {
                auto& msg = track->getEventPointer(i)->message;
                if (msg.isNoteOn())
                    timestamps.push_back(msg.getTimeStamp());
            }
        }
        return timestamps;
    }

private:
    PluginProcessor processor;
};

// ============================================================================
// Basic Export Tests
// ============================================================================

TEST_CASE("MidiExporter basic export") {
    ExporterFixture f;
    auto& state = f.getState();

    SECTION("Default 8-step sequence produces 8 note-on events") {
        // Default: 8 steps, each with 1 substep, note=60 (middle C)
        auto midi = f.exportMidi();
        REQUIRE(f.countNoteOns(midi) == 8);
    }

    SECTION("Exported MIDI file has 1 track") {
        auto midi = f.exportMidi();
        REQUIRE(midi.getNumTracks() == 1);
    }

    SECTION("Exported MIDI has correct PPQ") {
        auto midi = f.exportMidi();
        REQUIRE(midi.getTimeFormat() == MidiExporter::PPQ);
    }

    SECTION("No tempo meta-event is written (avoids DAW import dialogs)") {
        auto midi = f.exportMidi();
        auto* track = midi.getTrack(0);
        bool foundTempo = false;
        for (int i = 0; i < track->getNumEvents(); ++i) {
            if (track->getEventPointer(i)->message.isTempoMetaEvent()) {
                foundTempo = true;
                break;
            }
        }
        REQUIRE_FALSE(foundTempo);
    }
}

// ============================================================================
// Sequence Length Tests
// ============================================================================

TEST_CASE("MidiExporter respects sequence length") {
    ExporterFixture f;
    auto& state = f.getState();

    SECTION("3-step sequence produces exactly 3 notes") {
        state.setLength(3);
        auto midi = f.exportMidi();
        REQUIRE(f.countNoteOns(midi) == 3);
    }

    SECTION("1-step sequence produces exactly 1 note") {
        state.setLength(1);
        auto midi = f.exportMidi();
        REQUIRE(f.countNoteOns(midi) == 1);
    }

    SECTION("16-step sequence produces exactly 16 notes") {
        state.setLength(16);
        auto midi = f.exportMidi();
        REQUIRE(f.countNoteOns(midi) == 16);
    }

    SECTION("3-step sequence at 16th note subdivision has correct total length") {
        state.setLength(3);
        // Default subdivision=2 (16th), ticksPerStep = 480/4 = 120
        // 3 steps × 120 ticks = 360 ticks total
        // Last note-off should be at 360 - some portion (noteLength dependent)
        auto midi = f.exportMidi();
        auto timestamps = f.getNoteOnTimestamps(midi);
        REQUIRE(timestamps.size() == 3);
        REQUIRE(timestamps[0] == 0.0);
        REQUIRE(timestamps[1] == 120.0);
        REQUIRE(timestamps[2] == 240.0);
    }
}

// ============================================================================
// Substep Unrolling Tests
// ============================================================================

TEST_CASE("MidiExporter unrolls substeps correctly") {
    ExporterFixture f;
    auto& state = f.getState();

    SECTION("Step with 2 substeps produces 2 notes in export") {
        state.setLength(1);
        state.setStepNotesLength(0, 2);
        state.setStepNote(0, 0, 60);  // C4
        state.setStepNote(0, 1, 64);  // E4

        auto midi = f.exportMidi();
        REQUIRE(f.countNoteOns(midi) == 2);

        auto notes = f.getNoteOns(midi);
        REQUIRE(notes[0] == 60);
        REQUIRE(notes[1] == 64);
    }

    SECTION("Step with 3 substeps produces 3 equally-spaced notes") {
        state.setLength(1);
        state.setStepNotesLength(0, 3);
        state.setStepNote(0, 0, 60);
        state.setStepNote(0, 1, 62);
        state.setStepNote(0, 2, 64);

        auto midi = f.exportMidi();
        auto timestamps = f.getNoteOnTimestamps(midi);
        REQUIRE(timestamps.size() == 3);
        // Each substep gets ticksPerStep (120 at 16th)
        REQUIRE(timestamps[0] == 0.0);
        REQUIRE(timestamps[1] == 120.0);
        REQUIRE(timestamps[2] == 240.0);
    }

    SECTION("3 steps with varying substep counts unrolls correctly") {
        // Step 0: 2 substeps, Step 1: 1 substep, Step 2: 3 substeps
        // Total unrolled = 2 + 1 + 3 = 6 MIDI notes
        state.setLength(3);

        state.setStepNotesLength(0, 2);
        state.setStepNote(0, 0, 60);
        state.setStepNote(0, 1, 62);

        state.setStepNotesLength(1, 1);
        state.setStepNote(1, 0, 64);

        state.setStepNotesLength(2, 3);
        state.setStepNote(2, 0, 65);
        state.setStepNote(2, 1, 67);
        state.setStepNote(2, 2, 69);

        auto midi = f.exportMidi();
        REQUIRE(f.countNoteOns(midi) == 6);

        auto notes = f.getNoteOns(midi);
        REQUIRE(notes[0] == 60);
        REQUIRE(notes[1] == 62);
        REQUIRE(notes[2] == 64);
        REQUIRE(notes[3] == 65);
        REQUIRE(notes[4] == 67);
        REQUIRE(notes[5] == 69);

        // Timestamps: each unrolled step is 120 ticks
        auto timestamps = f.getNoteOnTimestamps(midi);
        REQUIRE(timestamps[0] == 0.0);
        REQUIRE(timestamps[1] == 120.0);   // step 0, sub 1
        REQUIRE(timestamps[2] == 240.0);   // step 1, sub 0
        REQUIRE(timestamps[3] == 360.0);   // step 2, sub 0
        REQUIRE(timestamps[4] == 480.0);   // step 2, sub 1
        REQUIRE(timestamps[5] == 600.0);   // step 2, sub 2
    }
}

// ============================================================================
// Rest Handling Tests
// ============================================================================

TEST_CASE("MidiExporter handles rests correctly") {
    ExporterFixture f;
    auto& state = f.getState();

    SECTION("Rest step (-1) produces no note-on but advances time") {
        state.setLength(3);
        state.setStepNote(0, 0, 60);
        state.setStepNote(1, 0, -1);  // rest
        state.setStepNote(2, 0, 64);

        auto midi = f.exportMidi();
        REQUIRE(f.countNoteOns(midi) == 2);

        auto notes = f.getNoteOns(midi);
        REQUIRE(notes[0] == 60);
        REQUIRE(notes[1] == 64);

        // Note timestamps should skip the rest step
        auto timestamps = f.getNoteOnTimestamps(midi);
        REQUIRE(timestamps[0] == 0.0);
        REQUIRE(timestamps[1] == 240.0);  // step 2 (step 1 was rest)
    }
}

// ============================================================================
// Subdivision Tests
// ============================================================================

TEST_CASE("MidiExporter respects subdivision setting") {
    ExporterFixture f;
    auto& state = f.getState();

    SECTION("Quarter note subdivision uses 480 ticks per step") {
        state.setLength(2);
        state.setClockSubdivision(0);  // quarter

        auto midi = f.exportMidi();
        auto timestamps = f.getNoteOnTimestamps(midi);
        REQUIRE(timestamps.size() == 2);
        REQUIRE(timestamps[0] == 0.0);
        REQUIRE(timestamps[1] == 480.0);
    }

    SECTION("Eighth note subdivision uses 240 ticks per step") {
        state.setLength(2);
        state.setClockSubdivision(1);  // eighth

        auto midi = f.exportMidi();
        auto timestamps = f.getNoteOnTimestamps(midi);
        REQUIRE(timestamps.size() == 2);
        REQUIRE(timestamps[0] == 0.0);
        REQUIRE(timestamps[1] == 240.0);
    }

    SECTION("32nd note subdivision uses 60 ticks per step") {
        state.setLength(2);
        state.setClockSubdivision(3);  // 32nd

        auto midi = f.exportMidi();
        auto timestamps = f.getNoteOnTimestamps(midi);
        REQUIRE(timestamps.size() == 2);
        REQUIRE(timestamps[0] == 0.0);
        REQUIRE(timestamps[1] == 60.0);
    }
}

// ============================================================================
// Scale Quantization in Export Tests
// ============================================================================

TEST_CASE("MidiExporter applies scale quantization") {
    ExporterFixture f;
    auto& state = f.getState();

    SECTION("Note outside scale is quantized to nearest scale note") {
        state.setLength(1);
        state.setStepNote(0, 0, 61);  // Db4

        // Set global scale to C major (only C D E F G A B enabled)
        for (int i = 0; i < 12; ++i)
            state.setGlobalScaleNote(i, false);
        state.setGlobalScaleNote(0, true);   // C
        state.setGlobalScaleNote(2, true);   // D
        state.setGlobalScaleNote(4, true);   // E
        state.setGlobalScaleNote(5, true);   // F
        state.setGlobalScaleNote(7, true);   // G
        state.setGlobalScaleNote(9, true);   // A
        state.setGlobalScaleNote(11, true);  // B

        auto midi = f.exportMidi();
        auto notes = f.getNoteOns(midi);
        REQUIRE(notes.size() == 1);
        // Db(61) should quantize down to C(60) (prefer lower when equidistant)
        REQUIRE(notes[0] == 60);
    }
}

// ============================================================================
// Note Range in Export Tests
// ============================================================================

TEST_CASE("MidiExporter respects note range") {
    ExporterFixture f;
    auto& state = f.getState();

    SECTION("Note outside range is clamped into range") {
        state.setLength(1);
        state.setStepNote(0, 0, 90);  // well above range

        // Set range to 60-72 (one octave around middle C)
        state.setScaleWindowOffset(60);
        state.setScaleWindowSize(13);  // 60 to 72 inclusive

        auto midi = f.exportMidi();
        auto notes = f.getNoteOns(midi);
        REQUIRE(notes.size() == 1);
        // Note 90 should be clamped to 72 (range max)
        REQUIRE(notes[0] == 72);
    }

    SECTION("Note below range is clamped up") {
        state.setLength(1);
        state.setStepNote(0, 0, 30);  // below range

        state.setScaleWindowOffset(60);
        state.setScaleWindowSize(13);

        auto midi = f.exportMidi();
        auto notes = f.getNoteOns(midi);
        REQUIRE(notes.size() == 1);
        REQUIRE(notes[0] == 60);  // clamped to range min
    }
}

// ============================================================================
// Write and Read Back Tests
// ============================================================================

TEST_CASE("MidiExporter write and read roundtrip") {
    ExporterFixture f;
    auto& state = f.getState();

    SECTION("Write to temp file and read back has same number of events") {
        state.setLength(4);
        auto midi = f.exportMidi();

        auto tempFile = MidiExporter::writeToTempFile(midi);
        REQUIRE(tempFile.existsAsFile());

        // Read it back
        juce::FileInputStream stream(tempFile);
        REQUIRE(stream.openedOk());

        juce::MidiFile readBack;
        REQUIRE(readBack.readFrom(stream));
        REQUIRE(readBack.getNumTracks() == 1);

        // Count note-ons in read-back file
        int noteOns = 0;
        auto* track = readBack.getTrack(0);
        for (int i = 0; i < track->getNumEvents(); ++i) {
            if (track->getEventPointer(i)->message.isNoteOn())
                ++noteOns;
        }
        REQUIRE(noteOns == 4);

        // Cleanup
        tempFile.deleteFile();
    }
}
