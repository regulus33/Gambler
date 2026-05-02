/**
 * DawSyncTests.cpp
 *
 * Behavioural tests for DAW_SYNC mode driven through a fake AudioPlayHead.
 *
 * Contract: in DAW_SYNC the plugin is *just clocked* by the host. A tick
 * fires only when the host PPQ moves forward continuously across a pulse
 * boundary. Anything else — a backwards jump (loop), a manual scrub, the
 * playhead being repositioned during stop, transport state changes — must
 * NOT produce spurious sequencer ticks.
 */

#include "helpers/TestApp.h"
#include <catch2/catch_test_macros.hpp>
#include <data/Params.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

namespace DawSyncTest {

// ---------------------------------------------------------------------------
// Fake host playhead.
//
// PluginProcessor pulls position via getPlayHead()->getPosition() each block.
// We just hand back whatever the test scripted.
// ---------------------------------------------------------------------------
class FakePlayHead : public juce::AudioPlayHead {
public:
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override {
        if (returnNullopt) return {};
        juce::AudioPlayHead::PositionInfo info;
        info.setIsPlaying(isPlaying);
        info.setPpqPosition(ppq);
        info.setBpm(bpm);
        return info;
    }

    bool   isPlaying     = false;
    double ppq           = 0.0;
    double bpm           = 120.0;
    bool   returnNullopt = false;
};

// Block size chosen so that one block of continuous play at 120 BPM
// covers ~one 1/16 pulse: 6240 samples / 48 kHz × 120/60 = 0.26 PPQ. Driving
// the fake playhead by +0.26 PPQ per block therefore mimics actual continuous
// playback (and clears the new "non-continuous delta = jump" detector).
inline constexpr double kSampleRate     = 48000.0;
inline constexpr int    kBlockSize      = 6240;
inline constexpr double kPpqPerBlock    = 0.26;

// ---------------------------------------------------------------------------
// Fixture priming: 16-step chromatic ascent on chromatic scale, DAW_SYNC mode,
// 1/16 subdivision, fake playhead attached. Sequencer is moved into the
// Playing state up front so that detectDawTransport's stopped→playing flip
// is not what governs ticking — the test wants to observe DawSync alone.
// ---------------------------------------------------------------------------
inline void prime(TestApp& app, FakePlayHead& playHead,
                  double sampleRate = kSampleRate, int blockSize = kBlockSize) {
    for (int pc = 0; pc < 12; ++pc)
        app.setBoolParam(Params::Keys::ID::globalScaleNote(pc), true);

    app.state().setLength(16);
    for (int i = 0; i < 16; ++i) {
        app.state().setStepNote(i, 0, 60 + i);
        app.state().setStepNotesLength(i, 1);
        app.state().setPlayProbability(i, true);
    }

    app.setIntParam(Params::Keys::ID::CLOCK_SUBDIVISION, 2); // 1/16
    app.setIntParam(Params::Keys::ID::CLOCK_MODE, 1);        // DAW_SYNC

    // setRateAndBufferSizeDetails populates AudioProcessor::getSampleRate(),
    // which DawSync needs to compute sub-block pulse offsets. prepareToPlay
    // alone only configures the Clock helper.
    app.processor().setRateAndBufferSizeDetails(sampleRate, blockSize);
    app.processor().prepareToPlay(sampleRate, blockSize);
    app.processor().setPlayHead(&playHead);
    app.processMessages();
}

// Drive one block; return the count of note-on events emitted (= pulses ticked).
inline int driveBlock(TestApp& app, int blockSize = kBlockSize) {
    juce::AudioBuffer<float> buffer(2, blockSize);
    juce::MidiBuffer midi;
    buffer.clear();
    app.processor().processBlock(buffer, midi);

    int noteOns = 0;
    for (const auto meta : midi)
        if (meta.getMessage().isNoteOn()) ++noteOns;
    return noteOns;
}

} // namespace DawSyncTest

using namespace DawSyncTest;

// ===========================================================================
// Sanity: continuous forward playback advances the sequencer one pulse at a time.
// ===========================================================================
SCENARIO("DAW continuous forward playback ticks the sequencer one step per pulse",
         "[user-story][daw-sync][continuous]") {
    GamblerTestInitialiser juceInit;

    GIVEN("DAW_SYNC at 1/16, fake playhead playing from PPQ 0") {
        TestApp app;
        FakePlayHead playHead;
        prime(app, playHead);

        playHead.isPlaying = true;
        playHead.ppq       = 0.0;

        WHEN("PPQ advances by ~one 1/16 (0.26) per block, 4 blocks total") {
            std::vector<int> pulsesPerBlock;
            for (int i = 0; i < 4; ++i) {
                playHead.ppq = (i + 1) * 0.26;  // 0.26, 0.52, 0.78, 1.04
                pulsesPerBlock.push_back(driveBlock(app));
            }

            THEN("Each block emits exactly one tick and the sequencer reaches step 3") {
                for (size_t i = 0; i < pulsesPerBlock.size(); ++i)
                    REQUIRE(pulsesPerBlock[i] == 1);
                REQUIRE(app.state().getCurrentIndex() == 3);
            }
        }
    }
}

// ===========================================================================
// Backwards jump (loop): no ticks fire on the jump itself, sequencer
// continues forward from where it was when the next pulse lands.
// ===========================================================================
SCENARIO("DAW loop-back does not tick the sequencer or move its index",
         "[user-story][daw-sync][loop-back]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Sequencer driven forward to step 3 by four DAW pulses") {
        TestApp app;
        FakePlayHead playHead;
        prime(app, playHead);

        playHead.isPlaying = true;
        for (int i = 0; i < 4; ++i) {
            playHead.ppq = (i + 1) * 0.26;
            driveBlock(app);
        }
        REQUIRE(app.state().getCurrentIndex() == 3);

        WHEN("DAW playhead loops backwards to PPQ 0") {
            playHead.ppq = 0.0;
            int loopBlockPulses = driveBlock(app);

            THEN("No ticks fire on the loop-back block") {
                REQUIRE(loopBlockPulses == 0);
                REQUIRE(app.state().getCurrentIndex() == 3);
            }

            AND_WHEN("Forward playback resumes from PPQ 0 and crosses one pulse") {
                playHead.ppq = 0.26;
                int nextPulses = driveBlock(app);

                THEN("Exactly one tick fires and the sequencer advances to step 4") {
                    REQUIRE(nextPulses == 1);
                    REQUIRE(app.state().getCurrentIndex() == 4);
                }
            }
        }
    }
}

// ===========================================================================
// Scrub-then-play: user moves the playhead while stopped, then presses play.
// First block of playback must NOT fire catch-up pulses to "match" the new
// PPQ position. Sequencer either stays put or fires at most one pulse if the
// new PPQ already crossed a boundary on its own.
// ===========================================================================
SCENARIO("DAW scrub-while-stopped + play does not catch-up the sequencer",
         "[user-story][daw-sync][scrub-then-play]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Sequencer driven to step 3, then DAW stops and user scrubs to PPQ 0.5") {
        TestApp app;
        FakePlayHead playHead;
        prime(app, playHead);

        playHead.isPlaying = true;
        for (int i = 0; i < 4; ++i) {
            playHead.ppq = (i + 1) * 0.26;
            driveBlock(app);
        }
        REQUIRE(app.state().getCurrentIndex() == 3);

        // DAW stops.
        playHead.isPlaying = false;
        playHead.ppq       = 1.04;
        driveBlock(app);

        // User scrubs the stopped playhead to a new position.
        playHead.ppq = 0.5;
        driveBlock(app);
        REQUIRE(app.state().getCurrentIndex() == 3);

        WHEN("DAW transport resumes from the scrubbed PPQ 0.5") {
            playHead.isPlaying = true;
            playHead.ppq       = 0.5;
            int firstPulses = driveBlock(app);

            THEN("First block emits no catch-up pulses — sequencer stays at step 3") {
                REQUIRE(firstPulses == 0);
                REQUIRE(app.state().getCurrentIndex() == 3);
            }
        }
    }
}

// ===========================================================================
// Forward seek during play: user nudges the playhead forward by 0.5 PPQ in
// a single block. That is not continuous playback — we MUST treat it as a
// jump (re-anchor, no pulses), not as 2 catch-up pulses.
// ===========================================================================
SCENARIO("DAW small forward seek during play does not catch-up the sequencer",
         "[user-story][daw-sync][forward-seek]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Sequencer driven to step 3 via continuous play to PPQ 1.04") {
        TestApp app;
        FakePlayHead playHead;
        prime(app, playHead);

        playHead.isPlaying = true;
        for (int i = 0; i < 4; ++i) {
            playHead.ppq = (i + 1) * 0.26;
            driveBlock(app);
        }
        REQUIRE(app.state().getCurrentIndex() == 3);

        WHEN("Playhead jumps forward 0.51 PPQ in a single block (manual seek)") {
            playHead.ppq = 1.04 + 0.51; // 1.55
            int seekBlockPulses = driveBlock(app);

            THEN("No catch-up pulses fire — sequencer stays at step 3") {
                REQUIRE(seekBlockPulses == 0);
                REQUIRE(app.state().getCurrentIndex() == 3);
            }

            AND_WHEN("Continuous playback resumes from PPQ 1.55 across one pulse") {
                playHead.ppq = 1.55 + 0.26;
                int nextPulses = driveBlock(app);

                THEN("Exactly one tick fires and sequencer advances to step 4") {
                    REQUIRE(nextPulses == 1);
                    REQUIRE(app.state().getCurrentIndex() == 4);
                }
            }
        }
    }
}
