/**
 * UserStoryTests.cpp - Comprehensive user-story integration tests
 *
 * Each SCENARIO simulates a real user workflow via the TestApp interface:
 *   app.navBar().clickPlayPause()
 *   app.sequencerControls().rollButton().press()
 *   app.state().getIsRandom()
 *
 * These tests verify that UI interactions drive the correct backend behaviour.
 */

#include "helpers/TestApp.h"
#include <catch2/catch_test_macros.hpp>
#include <data/Params.h>

// ============================================================================
// TRANSPORT
// ============================================================================

SCENARIO("Sequencer is playing on startup", "[user-story][transport]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A freshly launched app") {
        TestApp app;

        THEN("The sequencer is already playing") {
            REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Playing);
        }
    }
}

SCENARIO("User pauses and resumes playback from the same step", "[user-story][transport]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A playing sequencer that has advanced to step 5") {
        TestApp app;
        app.state().setCurrentIndex(5);
        REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Playing);

        WHEN("User clicks Play/Pause to pause") {
            app.navBar().clickPlayPause();
            app.processMessages();

            THEN("Sequencer is paused and stays on step 5") {
                REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Paused);
                REQUIRE(app.state().getCurrentIndex() == 5);
            }

            AND_WHEN("User clicks Play/Pause again to resume") {
                app.navBar().clickPlayPause();
                app.processMessages();

                THEN("Sequencer is playing again from the same step") {
                    REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Playing);
                    REQUIRE(app.state().getCurrentIndex() == 5);
                }
            }

            AND_WHEN("User clicks Stop instead of resuming") {
                app.navBar().clickStop();
                app.processMessages();

                THEN("Sequencer is stopped and index resets so next tick plays step 0") {
                    REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Stopped);
                    // Index is -1 (pre-start), meaning the next tick() will advance to step 0
                    REQUIRE(app.state().getCurrentIndex() == -1);
                }
            }
        }
    }
}

// ============================================================================
// DEFAULT STATE
// ============================================================================

SCENARIO("Fresh app has correct defaults", "[user-story][defaults]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A freshly launched app") {
        TestApp app;

        THEN("Sequence length is 8") {
            REQUIRE(app.state().getLength() == ParamConstants::DEFAULT_LENGTH);
        }

        AND_THEN("All 8 steps default to middle C (MIDI 60)") {
            for (int i = 0; i < 8; ++i) {
                REQUIRE(app.state().getStepNote(i, 0) == ParamConstants::DEFAULT_NOTE);
            }
        }
    }
}

// ============================================================================
// ROLL (Random Mode)
// ============================================================================

SCENARIO("Roll button randomizes notes as the sequencer plays through them", "[user-story][roll]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A playing sequencer with 8 default middle-C steps") {
        TestApp app;
        REQUIRE(app.state().getLength() == 8);
        app.state().setCurrentIndex(-1);

        // Enable all play probabilities so roll always generates notes (not rests)
        for (int i = 0; i < 64; ++i)
            app.state().setPlayProbability(i, true);

        WHEN("User presses Roll and the sequencer ticks through all 8 steps") {
            app.sequencerControls().rollButton().press();
            app.processMessages();
            REQUIRE(app.state().getIsRandom() == true);

            // Tick through all 8 steps, collecting the notes returned
            std::array<int, 8> playedNotes {};
            for (int i = 0; i < 8; ++i)
                playedNotes[i] = app.state().tick();

            // Flush the queued note writes to state
            app.state().applyPendingNoteWrites();

            THEN("At least one step has been changed from the default middle C") {
                bool anyChanged = false;
                for (int i = 0; i < 8; ++i) {
                    if (app.state().getStepNote(i, 0) != ParamConstants::DEFAULT_NOTE) {
                        anyChanged = true;
                        break;
                    }
                }
                REQUIRE(anyChanged);
            }

            AND_THEN("All played notes are within the scale range") {
                int rangeMin = app.state().getScaleRangeMin();
                int rangeMax = app.state().getScaleRangeMax();
                for (int i = 0; i < 8; ++i) {
                    if (playedNotes[i] >= 0) {
                        REQUIRE(playedNotes[i] >= rangeMin);
                        REQUIRE(playedNotes[i] <= rangeMax);
                    }
                }
            }

            AND_THEN("The stored notes match what was played") {
                for (int i = 0; i < 8; ++i) {
                    int stored = app.state().getStepNote(i, 0);
                    if (playedNotes[i] >= 0) {
                        // Stored note should be the unquantized version,
                        // and played note is quantized — but with chromatic scale
                        // (all notes enabled), they should be the same
                        REQUIRE(stored == playedNotes[i]);
                    }
                }
            }
        }
    }
}

SCENARIO("Roll button stops randomizing when released", "[user-story][roll]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A playing sequencer with roll engaged and notes randomized") {
        TestApp app;
        app.state().setCurrentIndex(-1);
        for (int i = 0; i < 64; ++i)
            app.state().setPlayProbability(i, true);

        app.sequencerControls().rollButton().press();
        app.processMessages();

        // Tick through once to randomize
        for (int i = 0; i < 8; ++i)
            app.state().tick();
        app.state().applyPendingNoteWrites();

        // Capture the notes after roll
        std::array<int, 8> notesAfterRoll {};
        for (int i = 0; i < 8; ++i)
            notesAfterRoll[i] = app.state().getStepNote(i, 0);

        WHEN("User releases Roll and ticks through the sequence again") {
            app.sequencerControls().rollButton().release();
            app.processMessages();
            REQUIRE(app.state().getIsRandom() == false);

            // Tick through again — should play stored notes, not generate new ones
            std::array<int, 8> playedNotes {};
            for (int i = 0; i < 8; ++i)
                playedNotes[i] = app.state().tick();

            THEN("The played notes are the same as the stored notes from the roll") {
                for (int i = 0; i < 8; ++i) {
                    REQUIRE(playedNotes[i] == notesAfterRoll[i]);
                }
            }
        }
    }
}

SCENARIO("MIDI-mapped transport controls pause, resume, and stop", "[user-story][transport][midi]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Play/Pause mapped to MIDI note 60, Stop mapped to MIDI note 61") {
        TestApp app;
        app.mapMidiNote(MidiControlId::PLAY_PAUSE, 60);
        app.mapMidiNote(MidiControlId::STOP, 61);

        app.state().setCurrentIndex(5);
        REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Playing);

        WHEN("MIDI note 60 is pressed and released to pause") {
            app.sendMidiNoteOn(60);
            app.sendMidiNoteOff(60);
            app.processMessages();

            THEN("Sequencer is paused and stays on step 5") {
                REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Paused);
                REQUIRE(app.state().getCurrentIndex() == 5);
            }

            AND_WHEN("MIDI note 60 is pressed and released again to resume") {
                app.sendMidiNoteOn(60);
                app.sendMidiNoteOff(60);
                app.processMessages();

                THEN("Sequencer is playing again from the same step") {
                    REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Playing);
                    REQUIRE(app.state().getCurrentIndex() == 5);
                }
            }

            AND_WHEN("MIDI note 61 is pressed and released to stop") {
                app.sendMidiNoteOn(61);
                app.sendMidiNoteOff(61);
                app.processMessages();

                THEN("Sequencer is stopped and index resets so next tick plays step 0") {
                    REQUIRE(app.processor().getSequencerState() == PluginProcessor::SequencerState::Stopped);
                    REQUIRE(app.state().getCurrentIndex() == -1);
                }
            }
        }
    }
}
