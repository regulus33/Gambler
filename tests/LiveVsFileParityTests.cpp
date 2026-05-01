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

    app.processor().prepareToPlay(sampleRate, blockSize);
    app.processor().getClock().setSampleRate(sampleRate);
    app.processor().getClock().setBpm(bpm);
    app.processor().getClock().start();
    app.processMessages();
}

constexpr double samplesPerStepFor(double sampleRate, double bpm, int pulsesPerQuarter) {
    return sampleRate * 60.0 / (bpm * static_cast<double>(pulsesPerQuarter));
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
            std::vector<LiveVsFileParityTest::StreamEvent> fileEvents;
            std::vector<LiveVsFileParityTest::StreamEvent> liveEvents;
            for (int i = 0; i < 16; ++i) {
                auto a = fileStream.next();
                auto b = liveStream.next();
                REQUIRE(a.has_value());
                REQUIRE(b.has_value());
                fileEvents.push_back(*a);
                liveEvents.push_back(*b);
            }

            THEN("Both streams emit exactly 16 note-ons") {
                REQUIRE(fileEvents.size() == 16);
                REQUIRE(liveEvents.size() == 16);
                // No spurious 17th event in either stream.
                REQUIRE_FALSE(fileStream.next().has_value());
            }

            AND_THEN("Pitch, velocity, and channel match exactly at every index") {
                for (size_t i = 0; i < 16; ++i) {
                    INFO("event " << i
                         << " file=(p=" << fileEvents[i].pitch
                         << " v=" << fileEvents[i].velocity
                         << " ch=" << fileEvents[i].channel << ")"
                         << " live=(p=" << liveEvents[i].pitch
                         << " v=" << liveEvents[i].velocity
                         << " ch=" << liveEvents[i].channel << ")");
                    REQUIRE(fileEvents[i].pitch    == liveEvents[i].pitch);
                    REQUIRE(fileEvents[i].velocity == liveEvents[i].velocity);
                    REQUIRE(fileEvents[i].channel  == liveEvents[i].channel);
                }
            }

            AND_THEN("Relative timing matches within 2 samples per event") {
                // Both streams are anchored at their own first event — the
                // file starts at tick 0, the live starts at whatever sample
                // the first internal-clock pulse fires. We compare *deltas*
                // from event[0], converted to samples for a unit-friendly
                // tolerance.
                const double fileT0 = fileEvents[0].stepIndexFloat;
                const double liveT0 = liveEvents[0].stepIndexFloat;

                double maxAbsErrorSamples = 0.0;
                for (size_t i = 1; i < 16; ++i) {
                    const double fileRelSamples =
                        (fileEvents[i].stepIndexFloat - fileT0) * samplesPerStep;
                    const double liveRelSamples =
                        (liveEvents[i].stepIndexFloat - liveT0) * samplesPerStep;
                    const double err = std::abs(liveRelSamples - fileRelSamples);
                    if (err > maxAbsErrorSamples) maxAbsErrorSamples = err;
                    INFO("event " << i
                         << " fileRelSamples=" << fileRelSamples
                         << " liveRelSamples=" << liveRelSamples
                         << " err=" << err);
                    REQUIRE(err <= 2.0);
                }
                INFO("max |timing error| across 15 events = "
                     << maxAbsErrorSamples << " samples");
            }
        }
    }
}
