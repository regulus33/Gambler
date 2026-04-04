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
