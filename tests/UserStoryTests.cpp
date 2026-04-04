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
