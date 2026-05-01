/**
 * LiveVsFileParityTests.cpp
 *
 * Differential oracle: the same deterministic sequence is rendered through
 * two paths (offline file export vs live processBlock output) and the
 * resulting note streams are walked in lock-step.
 *
 * The MidiExporter is the trusted reference — its output is what the user
 * drags into a DAW and expects to hear. The live path is the suspect: if
 * processBlock emits anything that does not match the exported file in
 * pitch/velocity/channel/relative timing, this test fails.
 *
 * Capture point on the live side is as far down the pipeline as the host
 * can see — the MidiBuffer returned from processBlock — so the comparison
 * reflects what a recording DAW would actually receive.
 */

#include "helpers/TestApp.h"
#include <catch2/catch_test_macros.hpp>
#include <data/Params.h>
#include <backend/MidiExporter.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <optional>
#include <set>
#include <vector>

namespace LiveVsFileParityTest {

// A single normalised event from either stream.
struct StreamEvent {
    int    channel        = 0;
    int    pitch          = 0;
    int    velocity       = 0;
    double stepIndexFloat = 0.0;  // event time expressed as fractional step index
};

// Common interface — file and live readers both yield events in firing order.
struct MidiEventStream {
    virtual ~MidiEventStream() = default;
    virtual std::optional<StreamEvent> next() = 0;
};

// Reads note-ons out of a juce::MidiFile track in tick order.
class FileMidiStream final : public MidiEventStream {
public:
    FileMidiStream(const juce::MidiFile& mf, double ticksPerStep)
        : ticksPerStep_(ticksPerStep) {
        if (mf.getNumTracks() > 0) {
            const auto* track = mf.getTrack(0);
            if (track != nullptr) {
                for (int i = 0; i < track->getNumEvents(); ++i) {
                    const auto* evt = track->getEventPointer(i);
                    if (evt->message.isNoteOn())
                        events_.push_back(&evt->message);
                }
            }
        }
    }

    std::optional<StreamEvent> next() override {
        if (cursor_ >= events_.size()) return std::nullopt;
        const auto& msg = *events_[cursor_++];
        StreamEvent e;
        e.channel        = msg.getChannel();
        e.pitch          = msg.getNoteNumber();
        e.velocity       = msg.getVelocity();
        e.stepIndexFloat = msg.getTimeStamp() / ticksPerStep_;
        return e;
    }

private:
    std::vector<const juce::MidiMessage*> events_;
    size_t cursor_       = 0;
    double ticksPerStep_;
};

// Drives a live PluginProcessor block-by-block, captures note-ons from each
// returned MidiBuffer, and yields them in firing order.
class LiveMidiStream final : public MidiEventStream {
public:
    LiveMidiStream(TestApp& app, int blockSize, double samplesPerStep, int maxBlocks)
        : app_(app),
          blockSize_(blockSize),
          samplesPerStep_(samplesPerStep),
          maxBlocks_(maxBlocks) {}

    std::optional<StreamEvent> next() override {
        while (queueCursor_ >= queue_.size() && blocksProcessed_ < maxBlocks_) {
            juce::AudioBuffer<float> buffer(2, blockSize_);
            juce::MidiBuffer midi;
            buffer.clear();
            app_.processor().processBlock(buffer, midi);

            const juce::int64 blockStart =
                static_cast<juce::int64>(blocksProcessed_) * blockSize_;
            for (const auto meta : midi) {
                if (!meta.getMessage().isNoteOn()) continue;
                StreamEvent e;
                e.channel        = meta.getMessage().getChannel();
                e.pitch          = meta.getMessage().getNoteNumber();
                e.velocity       = meta.getMessage().getVelocity();
                const juce::int64 absSample =
                    blockStart + static_cast<juce::int64>(meta.samplePosition);
                e.stepIndexFloat = static_cast<double>(absSample) / samplesPerStep_;
                queue_.push_back(e);
            }
            ++blocksProcessed_;
        }
        if (queueCursor_ >= queue_.size()) return std::nullopt;
        return queue_[queueCursor_++];
    }

private:
    TestApp&                  app_;
    int                       blockSize_;
    double                    samplesPerStep_;
    int                       maxBlocks_;
    int                       blocksProcessed_ = 0;
    std::vector<StreamEvent>  queue_;
    size_t                    queueCursor_     = 0;
};

// Build the same deterministic 16-step fixture in any TestApp instance.
inline void primeFixture(TestApp& app) {
    // Chromatic scale enabled so no quantisation moves notes around.
    for (int pc = 0; pc < 12; ++pc)
        app.setBoolParam(Params::Keys::ID::globalScaleNote(pc), true);

    app.state().setLength(16);
    for (int i = 0; i < 16; ++i) {
        app.state().setStepNote(i, 0, 60 + i);          // C4, C#4, D4, ...
        app.state().setStepNotesLength(i, 1);            // single substep
        app.state().setPlayProbability(i, true);
    }
}

inline void startLiveClock(TestApp& app, double sampleRate, int blockSize,
                           double bpm, int subdivisionParam) {
    app.setIntParam(Params::Keys::ID::CLOCK_MODE, 0); // INTERNAL
    app.setFloatParam(Params::Keys::ID::CLOCK_BPM, static_cast<float>(bpm));
    app.setIntParam(Params::Keys::ID::CLOCK_SUBDIVISION, subdivisionParam);

    // Mirror what togglePlayPause does on Stopped→Playing: reset the sequence
    // so currentIndex and every step's currentSubstep go to -1. Without this
    // they sit at the InternalState default of 0 and the first pre-incrementing
    // advanceSubstep skips over substep 0 — visible only on multi-substep
    // fixtures because mod-1 swallows the off-by-one for single-substep steps.
    app.state().reset();

    app.processor().prepareToPlay(sampleRate, blockSize);
    app.processor().getClock().setSampleRate(sampleRate);
    app.processor().getClock().setBpm(bpm);
    app.processor().getClock().start();
    app.processMessages();
}

constexpr double samplesPerStepFor(double sampleRate, double bpm, int pulsesPerQuarter) {
    return sampleRate * 60.0 / (bpm * static_cast<double>(pulsesPerQuarter));
}

// Walk both streams in lock-step for `expectedCount` events. Asserts:
//   - both streams yield exactly that many events
//   - pitch / velocity / channel match exactly at every index
//   - relative timing (delta from each stream's first event) matches
//     within `timingToleranceSamples` per event
//
// Defined as an inline function (not a SECTION) so callers can drive it
// from inside their own SCENARIO/GIVEN/WHEN/THEN blocks.
inline void compareStreams(MidiEventStream& fileStream,
                           MidiEventStream& liveStream,
                           int expectedCount,
                           double samplesPerStep,
                           double timingToleranceSamples = 2.0) {
    std::vector<StreamEvent> fileEvents;
    std::vector<StreamEvent> liveEvents;
    for (int i = 0; i < expectedCount; ++i) {
        auto a = fileStream.next();
        auto b = liveStream.next();
        INFO("collecting event " << i);
        REQUIRE(a.has_value());
        REQUIRE(b.has_value());
        fileEvents.push_back(*a);
        liveEvents.push_back(*b);
    }
    REQUIRE_FALSE(fileStream.next().has_value());

    for (int i = 0; i < expectedCount; ++i) {
        INFO("event " << i
             << " file=(p=" << fileEvents[(size_t) i].pitch
             << " v=" << fileEvents[(size_t) i].velocity
             << " ch=" << fileEvents[(size_t) i].channel << ")"
             << " live=(p=" << liveEvents[(size_t) i].pitch
             << " v=" << liveEvents[(size_t) i].velocity
             << " ch=" << liveEvents[(size_t) i].channel << ")");
        REQUIRE(fileEvents[(size_t) i].pitch    == liveEvents[(size_t) i].pitch);
        REQUIRE(fileEvents[(size_t) i].velocity == liveEvents[(size_t) i].velocity);
        REQUIRE(fileEvents[(size_t) i].channel  == liveEvents[(size_t) i].channel);
    }

    const double fileT0 = fileEvents.front().stepIndexFloat;
    const double liveT0 = liveEvents.front().stepIndexFloat;
    for (int i = 1; i < expectedCount; ++i) {
        const double fileRelSamples =
            (fileEvents[(size_t) i].stepIndexFloat - fileT0) * samplesPerStep;
        const double liveRelSamples =
            (liveEvents[(size_t) i].stepIndexFloat - liveT0) * samplesPerStep;
        const double err = std::abs(liveRelSamples - fileRelSamples);
        INFO("event " << i
             << " fileRelSamples=" << fileRelSamples
             << " liveRelSamples=" << liveRelSamples
             << " err=" << err);
        REQUIRE(err <= timingToleranceSamples);
    }
}

} // namespace LiveVsFileParityTest

// ============================================================================
// PARITY: live MIDI matches exported MIDI file note-for-note
// ============================================================================

SCENARIO("Live processBlock MIDI matches exported MIDI file note-for-note",
         "[user-story][parity][export-vs-live]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A deterministic 16-step chromatic ascent on channel 1") {
        constexpr double sampleRate        = 48000.0;
        constexpr int    blockSize         = 256;
        constexpr double bpm               = 120.0;
        constexpr int    subdivisionParam  = 2;   // 1/16
        constexpr int    pulsesPerQuarter  = 4;
        constexpr double ticksPerQuarter   = 480.0;
        constexpr double ticksPerStep      = ticksPerQuarter / pulsesPerQuarter; // 120

        // --- File path (reference) ---
        TestApp fileApp;
        LiveVsFileParityTest::primeFixture(fileApp);
        auto midiFile = MidiExporter::createMidiFile(fileApp.state());
        LiveVsFileParityTest::FileMidiStream fileStream(midiFile, ticksPerStep);

        // --- Live path (suspect) ---
        TestApp liveApp;
        LiveVsFileParityTest::primeFixture(liveApp);
        LiveVsFileParityTest::startLiveClock(liveApp, sampleRate, blockSize,
                                             bpm, subdivisionParam);

        const double samplesPerStep = LiveVsFileParityTest::samplesPerStepFor(
            sampleRate, bpm, pulsesPerQuarter);
        // Run for ~16.4 steps' worth of audio — enough to capture all 16
        // note-ons before the loop wraps back to step 0.
        const int totalSamples = static_cast<int>(samplesPerStep * 16.4);
        const int numBlocks    = (totalSamples + blockSize - 1) / blockSize;

        LiveVsFileParityTest::LiveMidiStream liveStream(liveApp, blockSize,
                                                        samplesPerStep, numBlocks);

        WHEN("the two streams are walked in lock-step") {
            THEN("All 16 events match in pitch/velocity/channel and timing") {
                LiveVsFileParityTest::compareStreams(fileStream, liveStream,
                                                      16, samplesPerStep);
            }
        }
    }
}

// ============================================================================
// Custom subdivision (1/8) — exercises pulsesPerQuarter=2 path
// ============================================================================

SCENARIO("Parity holds when subdivision is 1/8 instead of 1/16",
         "[user-story][parity][export-vs-live][subdivision]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Same 16-step chromatic fixture but at 1/8 subdivision") {
        constexpr double sampleRate        = 48000.0;
        constexpr int    blockSize         = 256;
        constexpr double bpm               = 120.0;
        constexpr int    subdivisionParam  = 1;   // 1 = EIGHTH
        constexpr int    pulsesPerQuarter  = 2;
        constexpr double ticksPerStep      = 480.0 / pulsesPerQuarter; // 240

        TestApp fileApp;
        LiveVsFileParityTest::primeFixture(fileApp);
        fileApp.setIntParam(Params::Keys::ID::CLOCK_SUBDIVISION, subdivisionParam);
        auto midiFile = MidiExporter::createMidiFile(fileApp.state());
        LiveVsFileParityTest::FileMidiStream fileStream(midiFile, ticksPerStep);

        TestApp liveApp;
        LiveVsFileParityTest::primeFixture(liveApp);
        LiveVsFileParityTest::startLiveClock(liveApp, sampleRate, blockSize,
                                             bpm, subdivisionParam);

        const double samplesPerStep = LiveVsFileParityTest::samplesPerStepFor(
            sampleRate, bpm, pulsesPerQuarter);
        const int totalSamples = static_cast<int>(samplesPerStep * 16.4);
        const int numBlocks    = (totalSamples + blockSize - 1) / blockSize;
        LiveVsFileParityTest::LiveMidiStream liveStream(liveApp, blockSize,
                                                        samplesPerStep, numBlocks);

        WHEN("both paths run with 1/8 subdivision") {
            THEN("All 16 events still match in pitch/velocity/channel and timing") {
                LiveVsFileParityTest::compareStreams(fileStream, liveStream,
                                                      16, samplesPerStep);
            }
        }
    }
}

// ============================================================================
// Custom MIDI output channel — exercises MIDI_OUTPUT_CHANNEL plumbing
// ============================================================================

SCENARIO("Parity holds when MIDI output channel is changed",
         "[user-story][parity][export-vs-live][channel]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Same fixture, MIDI output channel set to 7 on both apps") {
        constexpr double sampleRate        = 48000.0;
        constexpr int    blockSize         = 256;
        constexpr double bpm               = 120.0;
        constexpr int    subdivisionParam  = 2;
        constexpr int    pulsesPerQuarter  = 4;
        constexpr double ticksPerStep      = 480.0 / pulsesPerQuarter;
        constexpr int    customChannel     = 7;

        TestApp fileApp;
        LiveVsFileParityTest::primeFixture(fileApp);
        fileApp.setIntParam(Params::Keys::ID::MIDI_OUTPUT_CHANNEL, customChannel);
        auto midiFile = MidiExporter::createMidiFile(fileApp.state());
        LiveVsFileParityTest::FileMidiStream fileStream(midiFile, ticksPerStep);

        TestApp liveApp;
        LiveVsFileParityTest::primeFixture(liveApp);
        liveApp.setIntParam(Params::Keys::ID::MIDI_OUTPUT_CHANNEL, customChannel);
        LiveVsFileParityTest::startLiveClock(liveApp, sampleRate, blockSize,
                                             bpm, subdivisionParam);

        const double samplesPerStep = LiveVsFileParityTest::samplesPerStepFor(
            sampleRate, bpm, pulsesPerQuarter);
        const int totalSamples = static_cast<int>(samplesPerStep * 16.4);
        const int numBlocks    = (totalSamples + blockSize - 1) / blockSize;
        LiveVsFileParityTest::LiveMidiStream liveStream(liveApp, blockSize,
                                                        samplesPerStep, numBlocks);

        WHEN("both paths run with channel 7") {
            THEN("Both streams emit channel 7 throughout, in lock-step") {
                LiveVsFileParityTest::compareStreams(fileStream, liveStream,
                                                      16, samplesPerStep);
                // compareStreams checks that fileChannel == liveChannel; this
                // extra pass confirms it's the channel we asked for.
                LiveVsFileParityTest::FileMidiStream verifyStream(midiFile, ticksPerStep);
                while (auto e = verifyStream.next()) {
                    REQUIRE(e->channel == customChannel);
                }
            }
        }
    }
}

// ============================================================================
// Non-destructive scale quantisation — both paths must snap to the same
// pitches when the global scale narrows the available pitch classes.
// ============================================================================

SCENARIO("Parity holds when notes are non-destructively snapped to scale",
         "[user-story][parity][export-vs-live][scale]") {
    GamblerTestInitialiser juceInit;

    GIVEN("16 chromatic notes but the global scale is restricted to C major") {
        constexpr double sampleRate        = 48000.0;
        constexpr int    blockSize         = 256;
        constexpr double bpm               = 120.0;
        constexpr int    subdivisionParam  = 2;
        constexpr int    pulsesPerQuarter  = 4;
        constexpr double ticksPerStep      = 480.0 / pulsesPerQuarter;

        // C major: pitch classes {0, 2, 4, 5, 7, 9, 11} — i.e. C, D, E, F, G, A, B.
        const std::set<int> cMajor{0, 2, 4, 5, 7, 9, 11};

        auto applyCMajor = [&](TestApp& app) {
            for (int pc = 0; pc < 12; ++pc) {
                const bool enabled = cMajor.find(pc) != cMajor.end();
                app.setBoolParam(Params::Keys::ID::globalScaleNote(pc), enabled);
            }
        };

        TestApp fileApp;
        LiveVsFileParityTest::primeFixture(fileApp);
        applyCMajor(fileApp);
        auto midiFile = MidiExporter::createMidiFile(fileApp.state());
        LiveVsFileParityTest::FileMidiStream fileStream(midiFile, ticksPerStep);

        TestApp liveApp;
        LiveVsFileParityTest::primeFixture(liveApp);
        applyCMajor(liveApp);
        LiveVsFileParityTest::startLiveClock(liveApp, sampleRate, blockSize,
                                             bpm, subdivisionParam);

        const double samplesPerStep = LiveVsFileParityTest::samplesPerStepFor(
            sampleRate, bpm, pulsesPerQuarter);
        const int totalSamples = static_cast<int>(samplesPerStep * 16.4);
        const int numBlocks    = (totalSamples + blockSize - 1) / blockSize;
        LiveVsFileParityTest::LiveMidiStream liveStream(liveApp, blockSize,
                                                        samplesPerStep, numBlocks);

        WHEN("both paths run with C major as the active scale") {
            THEN("Every emitted pitch is in C major, and the two streams agree") {
                LiveVsFileParityTest::compareStreams(fileStream, liveStream,
                                                      16, samplesPerStep);

                // Walk the file again to confirm every pitch class is in C major.
                LiveVsFileParityTest::FileMidiStream verifyStream(midiFile, ticksPerStep);
                while (auto e = verifyStream.next()) {
                    INFO("pitch " << e->pitch << " (pc=" << (e->pitch % 12) << ")");
                    REQUIRE(cMajor.find(e->pitch % 12) != cMajor.end());
                }
            }
        }
    }
}

// ============================================================================
// Multi-substep parity — exercises LCM-based unrolling on the export side.
// Substep counts {1, 2, 3, 1} → LCM = 6 passes × 4 steps = 24 events.
// ============================================================================

SCENARIO("Parity holds when steps have varied substep counts (LCM unroll)",
         "[user-story][parity][export-vs-live][substep]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A 4-step sequence with substep counts [1, 2, 3, 1]") {
        constexpr double sampleRate        = 48000.0;
        constexpr int    blockSize         = 256;
        constexpr double bpm               = 120.0;
        constexpr int    subdivisionParam  = 2;   // 1/16
        constexpr int    pulsesPerQuarter  = 4;
        constexpr double ticksPerStep      = 480.0 / pulsesPerQuarter; // 120
        // LCM(1, 2, 3, 1) = 6 passes, 4 steps each → 24 events.
        constexpr int    expectedEvents    = 24;

        auto primeMultiSubstep = [](TestApp& app) {
            for (int pc = 0; pc < 12; ++pc)
                app.setBoolParam(Params::Keys::ID::globalScaleNote(pc), true);
            app.state().setLength(4);

            // Step 0: single substep, note 60.
            app.state().setStepNotesLength(0, 1);
            app.state().setStepNote(0, 0, 60);
            app.state().setPlayProbability(0, true);

            // Step 1: 2 substeps, notes 62, 64.
            app.state().setStepNotesLength(1, 2);
            app.state().setStepNote(1, 0, 62);
            app.state().setStepNote(1, 1, 64);
            app.state().setPlayProbability(1, true);

            // Step 2: 3 substeps, notes 65, 66, 67.
            app.state().setStepNotesLength(2, 3);
            app.state().setStepNote(2, 0, 65);
            app.state().setStepNote(2, 1, 66);
            app.state().setStepNote(2, 2, 67);
            app.state().setPlayProbability(2, true);

            // Step 3: single substep, note 69.
            app.state().setStepNotesLength(3, 1);
            app.state().setStepNote(3, 0, 69);
            app.state().setPlayProbability(3, true);
        };

        TestApp fileApp;
        primeMultiSubstep(fileApp);
        auto midiFile = MidiExporter::createMidiFile(fileApp.state());
        LiveVsFileParityTest::FileMidiStream fileStream(midiFile, ticksPerStep);

        TestApp liveApp;
        primeMultiSubstep(liveApp);
        LiveVsFileParityTest::startLiveClock(liveApp, sampleRate, blockSize,
                                             bpm, subdivisionParam);

        const double samplesPerStep = LiveVsFileParityTest::samplesPerStepFor(
            sampleRate, bpm, pulsesPerQuarter);
        // Run for one full LCM cycle plus a small buffer so the live stream
        // captures all 24 events before the cycle repeats.
        const int totalSamples = static_cast<int>(samplesPerStep * (expectedEvents + 1));
        const int numBlocks    = (totalSamples + blockSize - 1) / blockSize;
        LiveVsFileParityTest::LiveMidiStream liveStream(liveApp, blockSize,
                                                        samplesPerStep, numBlocks);

        WHEN("both paths run for one full LCM cycle") {
            THEN("Every event matches in pitch/velocity/channel and timing") {
                LiveVsFileParityTest::compareStreams(fileStream, liveStream,
                                                      expectedEvents, samplesPerStep);
            }

            AND_THEN("The 24-event sequence walks substep cursors deterministically") {
                // Verify the file's note pattern is what live would play:
                // pass 0 → 60, 62, 65, 69
                // pass 1 → 60, 64, 66, 69
                // pass 2 → 60, 62, 67, 69
                // pass 3 → 60, 64, 65, 69
                // pass 4 → 60, 62, 66, 69
                // pass 5 → 60, 64, 67, 69
                LiveVsFileParityTest::FileMidiStream verify(midiFile, ticksPerStep);
                const std::vector<int> expectedPitches = {
                    60, 62, 65, 69,
                    60, 64, 66, 69,
                    60, 62, 67, 69,
                    60, 64, 65, 69,
                    60, 62, 66, 69,
                    60, 64, 67, 69,
                };
                for (size_t i = 0; i < expectedPitches.size(); ++i) {
                    auto e = verify.next();
                    REQUIRE(e.has_value());
                    INFO("pass-event " << i
                         << " expected=" << expectedPitches[i]
                         << " got=" << e->pitch);
                    REQUIRE(e->pitch == expectedPitches[i]);
                }
                REQUIRE_FALSE(verify.next().has_value());
            }
        }
    }
}
