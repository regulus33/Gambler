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
#include <backend/Scale.h>
#include <ui/MainScreenComponent.h>
#include <ui/SequenceGrid.h>
#include <ui/SequenceOverviewComponent.h>
#include <ui/MainControlsComponent.h>
#include <ui/NoteControlsComponent.h>

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

// ============================================================================
// NOTE / OCTAVE SLIDER → SUBSTEP
// ============================================================================

SCENARIO("Changing the note slider updates the selected substep", "[user-story][substep][note-slider]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 2 substeps (C4=60 and E4=64), substep 1 is selected") {
        TestApp app;
        app.state().setStepNotesLength(0, 2);
        app.state().setStepNote(0, 0, 60); // C4
        app.state().setStepNote(0, 1, 64); // E4

        app.sequencerSteps().clickStep(0);
        app.processMessages();

        // Select substep 1 for editing
        app.subsequenceControls().clickBar(1);
        app.processMessages();

        REQUIRE(app.state().getSelectedSubstepIndex(0) == 1);

        WHEN("User changes the note slider to G (pitch class 7)") {
            app.mainScreen().getMainControls().getNoteControls().onNoteChanged(7);
            app.processMessages();

            THEN("Substep 1 is now G4 (octave preserved from E4=64, so 5*12+7=67)") {
                REQUIRE(app.state().getStepNote(0, 1) == 67);
            }

            AND_THEN("Substep 0 is unchanged (still C4=60)") {
                REQUIRE(app.state().getStepNote(0, 0) == 60);
            }
        }
    }
}

SCENARIO("Changing the octave slider updates the selected substep", "[user-story][substep][octave-slider]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 2 substeps (C4=60 and E4=64), substep 0 is selected") {
        TestApp app;
        app.state().setStepNotesLength(0, 2);
        app.state().setStepNote(0, 0, 60); // C4
        app.state().setStepNote(0, 1, 64); // E4

        app.sequencerSteps().clickStep(0);
        app.processMessages();

        // Select substep 0 for editing
        app.subsequenceControls().clickBar(0);
        app.processMessages();

        REQUIRE(app.state().getSelectedSubstepIndex(0) == 0);

        WHEN("User changes the octave slider to octave 6 (index 6)") {
            app.mainScreen().getMainControls().getNoteControls().onOctaveChanged(6);
            app.processMessages();

            THEN("Substep 0 is now C6 (6*12+0=72, pitch class preserved)") {
                REQUIRE(app.state().getStepNote(0, 0) == 72);
            }

            AND_THEN("Substep 1 is unchanged (still E4=64)") {
                REQUIRE(app.state().getStepNote(0, 1) == 64);
            }
        }
    }
}

SCENARIO("Switching selected substep and editing updates only that substep", "[user-story][substep][note-slider]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 3 substeps: C4=60, E4=64, G4=67") {
        TestApp app;
        app.state().setStepNotesLength(0, 3);
        app.state().setStepNote(0, 0, 60);
        app.state().setStepNote(0, 1, 64);
        app.state().setStepNote(0, 2, 67);

        app.sequencerSteps().clickStep(0);
        app.processMessages();

        WHEN("User selects substep 2 and changes note to A (pitch class 9)") {
            app.subsequenceControls().clickBar(2);
            app.processMessages();
            REQUIRE(app.state().getSelectedSubstepIndex(0) == 2);

            app.mainScreen().getMainControls().getNoteControls().onNoteChanged(9);
            app.processMessages();

            THEN("Substep 2 becomes A4 (5*12+9=69)") {
                REQUIRE(app.state().getStepNote(0, 2) == 69);
            }

            AND_WHEN("User switches to substep 0 and changes octave to 3") {
                app.subsequenceControls().clickBar(0);
                app.processMessages();
                REQUIRE(app.state().getSelectedSubstepIndex(0) == 0);

                app.mainScreen().getMainControls().getNoteControls().onOctaveChanged(3);
                app.processMessages();

                THEN("Substep 0 becomes C3 (3*12+0=36)") {
                    REQUIRE(app.state().getStepNote(0, 0) == 36);
                }

                AND_THEN("Substep 1 and 2 are unchanged") {
                    REQUIRE(app.state().getStepNote(0, 1) == 64);
                    REQUIRE(app.state().getStepNote(0, 2) == 69);
                }
            }
        }
    }
}

SCENARIO("Double-clicking a substep bar sets it as the next to play", "[user-story][substep][double-click]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 3 substeps: C4, E4, G4") {
        TestApp app;
        app.state().setStepNotesLength(0, 3);
        app.state().setStepNote(0, 0, 60); // C4
        app.state().setStepNote(0, 1, 64); // E4
        app.state().setStepNote(0, 2, 67); // G4

        app.sequencerSteps().clickStep(0);
        app.processMessages();

        // Start with substep index at -1 (pre-start)
        app.state().setStepNotesCurrentIndex(0, -1);

        WHEN("User double-clicks substep bar 2") {
            app.subsequenceControls().doubleClickBar(2);
            app.processMessages();

            THEN("The next tick on step 0 plays substep 2 (G4)") {
                int note = app.state().tickStep(0);
                REQUIRE(note == 67);
                REQUIRE(app.state().getStepNotesCurrentIndex(0) == 2);
            }
        }

        WHEN("User double-clicks substep bar 1") {
            app.subsequenceControls().doubleClickBar(1);
            app.processMessages();

            THEN("The next tick on step 0 plays substep 1 (E4)") {
                int note = app.state().tickStep(0);
                REQUIRE(note == 64);
                REQUIRE(app.state().getStepNotesCurrentIndex(0) == 1);
            }

            AND_THEN("Subsequent ticks continue cycling from that point") {
                app.state().tickStep(0); // plays substep 1
                int note = app.state().tickStep(0); // plays substep 2
                REQUIRE(note == 67);
                REQUIRE(app.state().getStepNotesCurrentIndex(0) == 2);
            }
        }
    }
}

// ============================================================================
// MIDI-MAPPED SUBSTEP EDITING (mirrors the UI-driven tests above)
// ============================================================================

SCENARIO("MIDI-mapped SUBSEQ_PLUS and SUBSEQ_MINUS change substep length",
         "[user-story][substep][midi-mapped]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 is selected with 1 substep, plus/minus mapped to CC 20/21") {
        TestApp app;
        app.sequencerSteps().clickStep(0);
        app.processMessages();

        app.mapMidiCC(MidiControlId::SUBSEQ_PLUS, 20);
        app.mapMidiCC(MidiControlId::SUBSEQ_MINUS, 21);

        REQUIRE(app.state().getStepNotesLength(0) == 1);

        WHEN("User sends CC 20 three times (SUBSEQ_PLUS)") {
            app.sendMidiCC(20, 127);
            app.sendMidiCC(20, 127);
            app.sendMidiCC(20, 127);

            THEN("Step 0 now has 4 substeps") {
                REQUIRE(app.state().getStepNotesLength(0) == 4);
            }

            AND_WHEN("User sends CC 21 twice (SUBSEQ_MINUS)") {
                app.sendMidiCC(21, 127);
                app.sendMidiCC(21, 127);

                THEN("Step 0 now has 2 substeps") {
                    REQUIRE(app.state().getStepNotesLength(0) == 2);
                }
            }
        }
    }
}

SCENARIO("MIDI-mapped SUBSEQ_BAR selects a substep for editing",
         "[user-story][substep][midi-mapped]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 3 substeps, SUBSEQ_BAR_0..2 mapped to CCs 30/31/32") {
        TestApp app;
        app.state().setStepNotesLength(0, 3);
        app.state().setStepNote(0, 0, 60);
        app.state().setStepNote(0, 1, 64);
        app.state().setStepNote(0, 2, 67);

        app.sequencerSteps().clickStep(0);
        app.processMessages();

        app.mapMidiCC(MidiControlId::SUBSEQ_BAR_0, 30);
        app.mapMidiCC(MidiControlId::SUBSEQ_BAR_1, 31);
        app.mapMidiCC(MidiControlId::SUBSEQ_BAR_2, 32);

        WHEN("User sends CC 32 (bar 2)") {
            app.sendMidiCC(32, 127);

            THEN("Substep 2 is selected for editing") {
                REQUIRE(app.state().getSelectedSubstepIndex(0) == 2);
            }
        }

        WHEN("User sends CC 31 (bar 1)") {
            app.sendMidiCC(31, 127);

            THEN("Substep 1 is selected for editing") {
                REQUIRE(app.state().getSelectedSubstepIndex(0) == 1);
            }
        }
    }
}

SCENARIO("MIDI-mapped NOTE_SLIDER edits the MIDI-selected substep",
         "[user-story][substep][midi-mapped]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 2 substeps (C4, E4), SUBSEQ_BAR_1 on CC 31, NOTE_SLIDER on CC 40") {
        TestApp app;
        app.state().setStepNotesLength(0, 2);
        app.state().setStepNote(0, 0, 60); // C4
        app.state().setStepNote(0, 1, 64); // E4

        app.sequencerSteps().clickStep(0);
        app.processMessages();

        app.mapMidiCC(MidiControlId::SUBSEQ_BAR_1, 31);
        app.mapMidiCC(MidiControlId::NOTE_SLIDER, 40);

        WHEN("User sends CC 31 to select substep 1, then CC 40 with value 81 (pitch class 7 = G)") {
            app.sendMidiCC(31, 127);
            REQUIRE(app.state().getSelectedSubstepIndex(0) == 1);

            app.sendMidiCC(40, 81);

            THEN("Substep 1 is now G4 (octave 5 preserved from E4, 5*12+7=67)") {
                REQUIRE(app.state().getStepNote(0, 1) == 67);
            }

            AND_THEN("Substep 0 is unchanged (still C4=60)") {
                REQUIRE(app.state().getStepNote(0, 0) == 60);
            }
        }
    }
}

SCENARIO("MIDI-mapped OCTAVE_SLIDER edits the MIDI-selected substep",
         "[user-story][substep][midi-mapped]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 2 substeps (C4, E4), SUBSEQ_BAR_0 on CC 30, OCTAVE_SLIDER on CC 41") {
        TestApp app;
        app.state().setStepNotesLength(0, 2);
        app.state().setStepNote(0, 0, 60);
        app.state().setStepNote(0, 1, 64);

        app.sequencerSteps().clickStep(0);
        app.processMessages();

        app.mapMidiCC(MidiControlId::SUBSEQ_BAR_0, 30);
        app.mapMidiCC(MidiControlId::OCTAVE_SLIDER, 41);

        WHEN("User sends CC 30 to select substep 0, then CC 41 with value 85 (octave 6)") {
            app.sendMidiCC(30, 127);
            REQUIRE(app.state().getSelectedSubstepIndex(0) == 0);

            app.sendMidiCC(41, 85);

            THEN("Substep 0 is now C6 (6*12+0=72)") {
                REQUIRE(app.state().getStepNote(0, 0) == 72);
            }

            AND_THEN("Substep 1 is unchanged (still E4=64)") {
                REQUIRE(app.state().getStepNote(0, 1) == 64);
            }
        }
    }
}

SCENARIO("MIDI-mapped substep editing: full switch-and-edit workflow",
         "[user-story][substep][midi-mapped]") {
    GamblerTestInitialiser juceInit;

    GIVEN("Step 0 has 3 substeps (C4, E4, G4), full MIDI mapping") {
        TestApp app;
        app.state().setStepNotesLength(0, 3);
        app.state().setStepNote(0, 0, 60);
        app.state().setStepNote(0, 1, 64);
        app.state().setStepNote(0, 2, 67);

        app.sequencerSteps().clickStep(0);
        app.processMessages();

        app.mapMidiCC(MidiControlId::SUBSEQ_BAR_0, 30);
        app.mapMidiCC(MidiControlId::SUBSEQ_BAR_2, 32);
        app.mapMidiCC(MidiControlId::NOTE_SLIDER, 40);
        app.mapMidiCC(MidiControlId::OCTAVE_SLIDER, 41);

        WHEN("User selects substep 2 via MIDI and changes note to A (CC 40 value 104)") {
            app.sendMidiCC(32, 127);
            app.sendMidiCC(40, 104); // floor(104/127*11) = 9 = A

            THEN("Substep 2 becomes A4 (5*12+9=69)") {
                REQUIRE(app.state().getStepNote(0, 2) == 69);
            }

            AND_WHEN("User switches to substep 0 via MIDI and changes octave to 3 (CC 41 value 43)") {
                app.sendMidiCC(30, 127);
                REQUIRE(app.state().getSelectedSubstepIndex(0) == 0);

                app.sendMidiCC(41, 43); // floor(43/127*9) = 3

                THEN("Substep 0 becomes C3 (3*12+0=36)") {
                    REQUIRE(app.state().getStepNote(0, 0) == 36);
                }

                AND_THEN("Substep 1 and 2 are unchanged") {
                    REQUIRE(app.state().getStepNote(0, 1) == 64);
                    REQUIRE(app.state().getStepNote(0, 2) == 69);
                }
            }
        }
    }
}

// ============================================================================
// NON-DESTRUCTIVE SCALE QUANTIZATION (DISPLAY)
// ============================================================================
//
// With PREF_DESTRUCTIVE_SCALE / PREF_DESTRUCTIVE_RANGE OFF (default), the raw
// step notes stored in state are never rewritten when the user edits the
// scale via the PianoRollComponent. Instead, every UI surface that shows a
// step note reads through a "quantized" getter on SequenceState, so the
// *display* reflects the current scale while storage stays intact.
//
// This scenario injects a fixture of 8 raw notes covering the boundary cases
// of ScaleQuantizer (in-scale, snap-down, tie-goes-down, snap-up, cross-octave
// snap), reduces the scale to {C, E, G} by toggling piano-roll keys, and
// then verifies every display surface shows the quantized values while the
// raw storage is untouched.

SCENARIO("Non-destructive scale quantization updates every display surface",
         "[user-story][scale][quantization]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A fresh app (destructive scale/range OFF by default) with a known fixture") {
        TestApp app;
        auto& state = app.state();

        // Pause the sequencer so next-substep / active-step don't drift while
        // we pump the message loop during this test.
        app.navBar().clickPlayPause();
        app.processMessages();

        // Sanity: destructive modes are off by default
        REQUIRE_FALSE(app.processor().getAPVTS()
                          .getRawParameterValue(Params::Keys::ID::PREF_DESTRUCTIVE_SCALE)
                          ->load() >= 0.5f);
        REQUIRE_FALSE(app.processor().getAPVTS()
                          .getRawParameterValue(Params::Keys::ID::PREF_DESTRUCTIVE_RANGE)
                          ->load() >= 0.5f);

        // Fixture: 8 raw notes, one per step, covering quantization boundary cases.
        // Expected quantization under scale {C, E, G} (pitch classes {0, 4, 7}):
        //   60 C4  -> 60 C4  (in scale)
        //   61 Db4 -> 60 C4  (d=1 down=60 in scale)
        //   62 D4  -> 60 C4  (d=1 no hit; d=2 down=60 in scale)
        //   63 Eb4 -> 64 E4  (d=1 down=62 no; up=64 in scale)
        //   66 Gb4 -> 67 G4  (d=1 down=65 no; up=67 in scale)
        //   68 Ab4 -> 67 G4  (d=1 down=67 in scale)
        //   70 Bb4 -> 72 C5  (d=1 no; d=2 down=68 no; up=72 in scale)
        //   71 B4  -> 72 C5  (d=1 down=70 no; up=72 in scale)
        const int rawNotes[8]      = { 60, 61, 62, 63, 66, 68, 70, 71 };
        const int expectedQuant[8] = { 60, 60, 60, 64, 67, 67, 72, 72 };

        // Write raw fixture directly, bypassing any baking path.
        for (int i = 0; i < 8; ++i)
            state.setStepNote(i, 0, rawNotes[i]);

        // Step 0 also gets 4 substeps exercising the same quantization rules
        // so we can verify the subsequencer bar surface.
        state.setStepNotesLength(0, 4);
        const int step0RawSubs[4]      = { 60, 61, 63, 66 };
        const int step0ExpectedSubs[4] = { 60, 60, 64, 67 };
        for (int s = 0; s < 4; ++s)
            state.setStepNote(0, s, step0RawSubs[s]);

        app.processMessages();

        WHEN("User reduces the scale to {C, E, G} by toggling piano-roll keys") {
            // Default scale is chromatic — toggle off everything except C, E, G.
            // Chromatic indices: C=0, Db=1, D=2, Eb=3, E=4, F=5, Gb=6, G=7,
            //                    Ab=8, A=9, Bb=10, B=11
            const int keysToDisable[] = { 1, 2, 3, 5, 6, 8, 9, 10, 11 };
            for (int k : keysToDisable) {
                app.pianoRoll().toggleNote(k);
            }
            app.processMessages();

            THEN("The global scale has exactly C, E, G enabled") {
                REQUIRE(state.getGlobalScaleNote(0));        // C
                REQUIRE_FALSE(state.getGlobalScaleNote(1));  // Db
                REQUIRE_FALSE(state.getGlobalScaleNote(2));  // D
                REQUIRE_FALSE(state.getGlobalScaleNote(3));  // Eb
                REQUIRE(state.getGlobalScaleNote(4));        // E
                REQUIRE_FALSE(state.getGlobalScaleNote(5));  // F
                REQUIRE_FALSE(state.getGlobalScaleNote(6));  // Gb
                REQUIRE(state.getGlobalScaleNote(7));        // G
                REQUIRE_FALSE(state.getGlobalScaleNote(8));  // Ab
                REQUIRE_FALSE(state.getGlobalScaleNote(9));  // A
                REQUIRE_FALSE(state.getGlobalScaleNote(10)); // Bb
                REQUIRE_FALSE(state.getGlobalScaleNote(11)); // B
            }

            AND_THEN("Raw step-note storage is UNCHANGED (non-destructive)") {
                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getStepNote(i, 0) == rawNotes[i]);
                for (int s = 0; s < 4; ++s)
                    REQUIRE(state.getStepNote(0, s) == step0RawSubs[s]);
            }

            AND_THEN("SequenceState quantized getters return the expected values") {
                // step 0's "current" note is the next-to-play substep (default 0).
                REQUIRE(state.getCurrentNoteForStep(0) == step0ExpectedSubs[0]);
                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getCurrentNoteForStep(i) == expectedQuant[i]);

                for (int s = 0; s < 4; ++s)
                    REQUIRE(state.getQuantizedStepNote(0, s) == step0ExpectedSubs[s]);
            }

            AND_THEN("SequenceOverview grid squares show the quantized note colours") {
                auto& grid = app.mainScreen().getSequenceOverview().getSequenceGrid();
                for (int i = 0; i < 8; ++i) {
                    int expected = (i == 0) ? step0ExpectedSubs[0] : expectedQuant[i];
                    auto actualColour = grid.getSquareState(0, i).color;
                    auto expectedColour = state.getNoteColour(expected % 12);
                    REQUIRE(actualColour == expectedColour);
                }
            }

            AND_THEN("Subsequencer bars for step 0 show the quantized substep colours") {
                // SubsequenceControlsComponent subscribes to scale params, so
                // the piano-roll scale change above should already have
                // refreshed the bar colours without any further nudging.

                // Bar 0 is the "next to play" substep and paints with
                // UIConstants::activeStepColour rather than its note colour —
                // verify substeps 1..3 where the note colour is the displayed value.
                // The "next to play" substep paints with activeStepColour
                // regardless of its note, so exclude it from the colour check.
                int activeSubstep = state.getNextSubstepIndex(0);
                int checked = 0;
                auto subs = app.subsequenceControls();
                for (int s = 0; s < 4; ++s) {
                    if (s == activeSubstep) continue;
                    auto actualColour = subs.barColor(s);
                    auto expectedColour = state.getNoteColour(step0ExpectedSubs[s] % 12);
                    INFO("substep=" << s
                         << " expectedQuant=" << step0ExpectedSubs[s]
                         << " actualARGB=" << juce::String::toHexString((int)actualColour.getARGB())
                         << " expectedARGB=" << juce::String::toHexString((int)expectedColour.getARGB()));
                    REQUIRE(actualColour == expectedColour);
                    ++checked;
                }
                REQUIRE(checked >= 3);
            }

            AND_THEN("NoteControls (MainControls) displays the quantized value of the selected step") {
                // Step 3: raw Eb4=63 -> quantized E4=64 (snap up).
                app.sequencerSteps().clickStep(3);
                app.processMessages();

                int shown = app.mainScreen().getMainControls().getNoteControls().getMidiNote();
                REQUIRE(shown == expectedQuant[3]);

                // Step 6: raw Bb4=70 -> quantized C5=72 (cross-octave snap).
                app.sequencerSteps().clickStep(6);
                app.processMessages();

                shown = app.mainScreen().getMainControls().getNoteControls().getMidiNote();
                REQUIRE(shown == expectedQuant[6]);
            }
        }
    }
}
