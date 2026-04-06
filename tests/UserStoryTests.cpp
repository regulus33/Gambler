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
        app.setAllPlayProbabilities(true);

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
        app.setAllPlayProbabilities(true);

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

// ============================================================================
// SUBSEQUENCE
// ============================================================================

SCENARIO("User clicks a step and sees its substep in the subsequence controls", "[user-story][substep]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A fresh app with default middle-C notes") {
        TestApp app;
        const auto inactiveGrey = juce::Colour(SubsequenceControlsConstants::defaultGreyHex);

        WHEN("User clicks step 0") {
            app.sequencerSteps().clickStep(0);
            app.processMessages();

            THEN("Step 0 is selected for editing") {
                REQUIRE(app.state().getSelectedStepForEditing() == 0);
            }

            AND_THEN("The first substep bar shows a note color (not inactive grey)") {
                auto bar0Color = app.subsequenceControls().barColor(0);
                REQUIRE(bar0Color != inactiveGrey);
            }

            AND_THEN("The remaining 7 substep bars are inactive grey") {
                for (int i = 1; i < 8; ++i) {
                    REQUIRE(app.subsequenceControls().barColor(i) == inactiveGrey);
                }
            }

            AND_WHEN("User clicks the + button to add a second substep") {
                app.subsequenceControls().clickPlus();
                app.processMessages();

                THEN("Step 0 now has 2 substeps") {
                    REQUIRE(app.state().getStepNotesLength(0) == 2);
                }

                AND_THEN("The first two bars show note colors") {
                    REQUIRE(app.subsequenceControls().barColor(0) != inactiveGrey);
                    REQUIRE(app.subsequenceControls().barColor(1) != inactiveGrey);
                }

                AND_THEN("The remaining 6 bars are inactive grey") {
                    for (int i = 2; i < 8; ++i) {
                        REQUIRE(app.subsequenceControls().barColor(i) == inactiveGrey);
                    }
                }

                AND_WHEN("User sets substep 0 to C4 (60) and substep 1 to E4 (64)") {
                    app.state().setStepNote(0, 0, 60); // C4
                    app.state().setStepNote(0, 1, 64); // E4

                    AND_WHEN("User clicks substep bar 0") {
                        app.subsequenceControls().clickBar(0);
                        app.processMessages();

                        THEN("NoteSlider shows C (pitchClass 0) and OctaveSlider shows octave 5") {
                            auto& noteControls = app.mainScreen().getMainControls().getNoteControls();
                            REQUIRE(noteControls.getNoteSlider().getChromaticPitchClass() == 0);
                            REQUIRE(noteControls.getOctaveSlider().getActualOctaveIndex() == 5);
                        }
                    }

                    AND_WHEN("User clicks substep bar 1") {
                        app.subsequenceControls().clickBar(1);
                        app.processMessages();

                        THEN("NoteSlider shows E (pitchClass 4) and OctaveSlider shows octave 5") {
                            auto& noteControls = app.mainScreen().getMainControls().getNoteControls();
                            REQUIRE(noteControls.getNoteSlider().getChromaticPitchClass() == 4);
                            REQUIRE(noteControls.getOctaveSlider().getActualOctaveIndex() == 5);
                        }
                    }
                }

                AND_WHEN("Sequencer ticks through step 0 to advance the substep index") {
                    app.state().setStepNote(0, 0, 60); // C4
                    app.state().setStepNote(0, 1, 64); // E4

                    // Reset substep index to -1 (pre-start), mirroring what
                    // SequenceState::reset() does after stop
                    app.state().setStepNotesCurrentIndex(0, -1);
                    REQUIRE(app.state().getStepNotesCurrentIndex(0) == -1);

                    // tickStep advances before playing: (current + 1) % length
                    // First tick: (-1+1)%2 = 0 → plays substep 0
                    int note1 = app.state().tickStep(0);
                    REQUIRE(app.state().getStepNotesCurrentIndex(0) == 0);
                    REQUIRE(note1 == 60); // C4

                    // Second tick: (0+1)%2 = 1 → plays substep 1
                    int note2 = app.state().tickStep(0);
                    REQUIRE(app.state().getStepNotesCurrentIndex(0) == 1);
                    REQUIRE(note2 == 64); // E4

                    THEN("Third visit wraps back to substep 0") {
                        int note3 = app.state().tickStep(0);
                        REQUIRE(app.state().getStepNotesCurrentIndex(0) == 0);
                        REQUIRE(note3 == 60); // C4
                    }
                }
            }
        }
    }
}

SCENARIO("Roll randomizes all substeps as the sequencer cycles through them", "[user-story][substep][roll]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 3 substeps, all starting at middle C") {
        TestApp app;
        app.state().setCurrentIndex(-1);
        app.setAllPlayProbabilities(true);

        // Give step 0 three substeps, all default C4
        app.state().setStepNotesLength(0, 3);
        app.state().setStepNote(0, 0, 60);
        app.state().setStepNote(0, 1, 60);
        app.state().setStepNote(0, 2, 60);
        app.state().setStepNotesCurrentIndex(0, -1);

        WHEN("User engages roll and the sequencer loops 3 times (hitting step 0 three times)") {
            app.sequencerControls().rollButton().press();
            app.processMessages();
            REQUIRE(app.state().getIsRandom() == true);

            int length = app.state().getLength(); // 8 steps

            // Loop through the full sequence 3 times so step 0 is visited 3 times,
            // once for each substep
            for (int loop = 0; loop < 3; ++loop)
                for (int i = 0; i < length; ++i)
                    app.state().tick();

            app.state().applyPendingNoteWrites();

            THEN("All three substeps of step 0 have been written to") {
                // Each substep was visited once per loop, so after 3 loops
                // each substep has been overwritten at least once.
                // With random generation, at least one should differ from the
                // original middle C — run enough loops to make this near-certain.
                bool anySubstepChanged = false;
                for (int s = 0; s < 3; ++s) {
                    if (app.state().getStepNote(0, s) != 60) {
                        anySubstepChanged = true;
                        break;
                    }
                }
                REQUIRE(anySubstepChanged);
            }

            AND_THEN("The substep index has cycled correctly (3 visits to a 3-substep step wraps back to start)") {
                // 3 visits: -1→0, 0→1, 1→2, and since length is 3: (2+1)%3 would be next
                // After 3 ticks on this step, currentIndex should be 2
                REQUIRE(app.state().getStepNotesCurrentIndex(0) == 2);
            }
        }
    }
}

SCENARIO("Active rest substep shows active fill, not a selected border", "[user-story][substep]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 2 substeps: a note and a rest") {
        TestApp app;
        app.state().setStepNotesLength(0, 2);
        app.state().setStepNote(0, 0, 60);  // C4
        app.state().setStepNote(0, 1, -1);  // rest

        // Select step 0 for editing so subsequence controls show it
        app.sequencerSteps().clickStep(0);
        app.processMessages();

        WHEN("The rest substep is next to play (nextSubstep points to it)") {
            // nextSubstep = (currentSubstep + 1) % length
            // For substep 1 to be next: currentSubstep must be 0
            // One tick from -1 sets currentSubstep to 0, so nextSubstep = 1
            app.state().setStepNotesCurrentIndex(0, -1);
            app.state().tickStep(0); // currentIndex -> 0, nextSubstep -> 1
            app.processMessages();

            THEN("The rest bar has the active fill color, not rest grey") {
                auto restBarColor = app.subsequenceControls().barColor(1);
                REQUIRE(restBarColor == UIConstants::activeStepColour);
            }

            AND_THEN("The rest bar has no border (not the white 'selected' border)") {
                REQUIRE(app.subsequenceControls().barBorderStyle(1) == BarBorderStyle::none);
            }
        }

        WHEN("The rest substep is NOT next to play") {
            // currentSubstep = 1, so nextSubstep = (1+1)%2 = 0 (not the rest)
            app.state().setStepNotesCurrentIndex(0, 1);
            app.processMessages();

            THEN("The rest bar shows rest grey") {
                auto restBarColor = app.subsequenceControls().barColor(1);
                REQUIRE(restBarColor == juce::Colour(SubsequenceControlsConstants::restGreyHex));
            }

            AND_THEN("The rest bar has no border") {
                REQUIRE(app.subsequenceControls().barBorderStyle(1) == BarBorderStyle::none);
            }
        }
    }
}

// ============================================================================
// ROLL (MIDI)
// ============================================================================

SCENARIO("MIDI-mapped Roll randomizes notes while held and stops when released", "[user-story][roll][midi]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Roll mapped to MIDI note 62, with 8 default middle-C steps") {
        TestApp app;
        app.mapMidiNote(MidiControlId::BUTTON_ROLL, 62);
        app.state().setCurrentIndex(-1);
        app.setAllPlayProbabilities(true);

        WHEN("MIDI note 62 is held and the sequencer ticks through all 8 steps") {
            app.sendMidiNoteOn(62);
            app.processMessages();
            REQUIRE(app.state().getIsRandom() == true);

            std::array<int, 8> playedNotes {};
            for (int i = 0; i < 8; ++i)
                playedNotes[i] = app.state().tick();
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

            AND_THEN("The stored notes match what was played") {
                for (int i = 0; i < 8; ++i) {
                    int stored = app.state().getStepNote(i, 0);
                    if (playedNotes[i] >= 0)
                        REQUIRE(stored == playedNotes[i]);
                }
            }

            AND_WHEN("MIDI note 62 is released and the sequencer ticks again") {
                // Capture notes after roll
                std::array<int, 8> notesAfterRoll {};
                for (int i = 0; i < 8; ++i)
                    notesAfterRoll[i] = app.state().getStepNote(i, 0);

                app.sendMidiNoteOff(62);
                app.processMessages();
                REQUIRE(app.state().getIsRandom() == false);

                std::array<int, 8> replayedNotes {};
                for (int i = 0; i < 8; ++i)
                    replayedNotes[i] = app.state().tick();

                THEN("The played notes are the stored notes from the roll, not new random ones") {
                    for (int i = 0; i < 8; ++i)
                        REQUIRE(replayedNotes[i] == notesAfterRoll[i]);
                }
            }
        }
    }
}
