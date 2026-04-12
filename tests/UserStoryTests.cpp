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
#include <ui/sliders/NoteSlider.h>
#include <ui/sliders/OctaveSlider.h>
#include <ui/sliders/NotchedSlider.h>
#include <set>

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
            REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Playing);
        }
    }
}

SCENARIO("User pauses and resumes playback from the same step", "[user-story][transport]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A playing sequencer that has advanced to step 5") {
        TestApp app;
        app.state().setCurrentIndex(5);
        REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Playing);

        WHEN("User clicks Play/Pause to pause") {
            app.navBar().clickPlayPause();
            app.processMessages();

            THEN("Sequencer is paused and stays on step 5") {
                REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Paused);
                REQUIRE(app.state().getCurrentIndex() == 5);
            }

            AND_WHEN("User clicks Play/Pause again to resume") {
                app.navBar().clickPlayPause();
                app.processMessages();

                THEN("Sequencer is playing again from the same step") {
                    REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Playing);
                    REQUIRE(app.state().getCurrentIndex() == 5);
                }
            }

            AND_WHEN("User clicks Stop instead of resuming") {
                app.navBar().clickStop();
                app.processMessages();

                THEN("Sequencer is stopped and index resets so next tick plays step 0") {
                    REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Stopped);
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
        REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Playing);

        WHEN("MIDI note 60 is pressed and released to pause") {
            app.sendMidiNoteOn(60);
            app.sendMidiNoteOff(60);
            app.processMessages();

            THEN("Sequencer is paused and stays on step 5") {
                REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Paused);
                REQUIRE(app.state().getCurrentIndex() == 5);
            }

            AND_WHEN("MIDI note 60 is pressed and released again to resume") {
                app.sendMidiNoteOn(60);
                app.sendMidiNoteOff(60);
                app.processMessages();

                THEN("Sequencer is playing again from the same step") {
                    REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Playing);
                    REQUIRE(app.state().getCurrentIndex() == 5);
                }
            }

            AND_WHEN("MIDI note 61 is pressed and released to stop") {
                app.sendMidiNoteOn(61);
                app.sendMidiNoteOff(61);
                app.processMessages();

                THEN("Sequencer is stopped and index resets so next tick plays step 0") {
                    REQUIRE(app.processor().getSequencerState() == PluginProcessor::PlaybackState::Stopped);
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

        WHEN("The rest substep is the current substep") {
            // Set currentSubstep directly to 1 (the rest)
            app.state().setStepNotesCurrentIndex(0, 1);
            app.processMessages();

            THEN("The rest bar has the active fill color, not rest grey") {
                auto restBarColor = app.subsequenceControls().barColor(1);
                REQUIRE(restBarColor == UIConstants::activeStepColour);
            }

            AND_THEN("The rest bar has no border (not the white 'selected' border)") {
                REQUIRE(app.subsequenceControls().barBorderStyle(1) == BarBorderStyle::none);
            }
        }

        WHEN("The rest substep is NOT the current substep") {
            // currentSubstep = 0, so the rest at index 1 is not active
            app.state().setStepNotesCurrentIndex(0, 0);
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

SCENARIO("Double-clicking a substep bar sets it as the current substep", "[user-story][substep][double-click]") {
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

            THEN("Substep 2 (G4) is now the current substep") {
                REQUIRE(app.state().getStepNotesCurrentIndex(0) == 2);
                REQUIRE(app.state().getCurrentNoteForStep(0) == 67);
            }

            AND_THEN("The next tick advances to substep 0 (C4)") {
                int note = app.state().tickStep(0);
                REQUIRE(note == 60);
                REQUIRE(app.state().getStepNotesCurrentIndex(0) == 0);
            }
        }

        WHEN("User double-clicks substep bar 1") {
            app.subsequenceControls().doubleClickBar(1);
            app.processMessages();

            THEN("Substep 1 (E4) is now the current substep") {
                REQUIRE(app.state().getStepNotesCurrentIndex(0) == 1);
                REQUIRE(app.state().getCurrentNoteForStep(0) == 64);
            }

            AND_THEN("Subsequent ticks continue cycling from that point") {
                int note = app.state().tickStep(0); // advances to substep 2
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

        // Pause the sequencer so active-step doesn't drift while
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
                // step 0's "current" note is the active substep (default 0).
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

                // The current substep paints with activeStepColour rather than
                // its note colour — exclude it from the colour check.
                int activeSubstep = state.getStepNotesCurrentIndex(0);
                if (activeSubstep < 0) activeSubstep = 0;
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

// ============================================================================
// NON-DESTRUCTIVE SCALE QUANTIZATION (PLAYBACK)
// ============================================================================
//
// Sister scenario to the display-surface test above. Here we verify that what
// the sequencer actually *plays* (via state.tick() — the exact call
// PluginProcessor::processBlock makes to get the next MIDI note) matches the
// displayed quantized values, and that the PianoRollComponent "active note"
// dot highlights the quantized pitch class of the currently-playing step.

SCENARIO("Non-destructive scale quantization drives playback and active-note dot",
         "[user-story][scale][quantization][playback]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A paused app with raw step notes covering quantization boundary cases") {
        TestApp app;
        auto& state = app.state();

        // Pause so nothing ticks behind our back during message pumping.
        app.navBar().clickPlayPause();
        app.processMessages();

        // Reset playhead to -1 so the first tick() lands on step 0.
        state.setCurrentIndex(-1);

        // Simple 1-substep-per-step fixture (same raw notes as the display test).
        const int rawNotes[8]      = { 60, 61, 62, 63, 66, 68, 70, 71 };
        const int expectedQuant[8] = { 60, 60, 60, 64, 67, 67, 72, 72 };

        for (int i = 0; i < 8; ++i) {
            state.setStepNotesLength(i, 1);
            state.setStepNote(i, 0, rawNotes[i]);
        }
        app.processMessages();

        WHEN("The scale is reduced to {C, E, G} via the piano roll") {
            const int keysToDisable[] = { 1, 2, 3, 5, 6, 8, 9, 10, 11 };
            for (int k : keysToDisable)
                app.pianoRoll().toggleNote(k);
            app.processMessages();

            THEN("state.tick() — the note processBlock would emit — matches the displayed quantized value at every step") {
                // tick() advances currentIndex by 1 (mod length) and returns
                // the quantized note for the newly-current step. After reset
                // to -1, eight ticks walk steps 0..7 in order.
                for (int i = 0; i < 8; ++i) {
                    int played = state.tick();
                    INFO("step=" << i << " raw=" << rawNotes[i]
                         << " played=" << played << " expected=" << expectedQuant[i]);
                    REQUIRE(played == expectedQuant[i]);
                    REQUIRE(state.getCurrentIndex() == i);
                }
            }

            AND_THEN("The piano-roll active-note dot follows the quantized pitch class as the playhead moves") {
                auto& pianoRoll = app.mainScreen().getPianoRoll();

                for (int i = 0; i < 8; ++i) {
                    state.setCurrentIndex(i);
                    // Fire the same callback that PluginProcessor::processBlock
                    // invokes after each sequencer tick — this is what pushes
                    // the new playhead position into the UI in production.
                    if (app.processor().onSequenceTick)
                        app.processor().onSequenceTick();
                    app.processMessages();

                    int expectedChromatic = expectedQuant[i] % 12;
                    int actualChromatic = pianoRoll.getActiveNote();
                    INFO("step=" << i
                         << " raw=" << rawNotes[i]
                         << " expectedQuant=" << expectedQuant[i]
                         << " expectedChromatic=" << expectedChromatic
                         << " actualChromatic=" << actualChromatic);
                    REQUIRE(actualChromatic == expectedChromatic);
                }
            }
        }
    }
}

// ============================================================================
// NON-DESTRUCTIVE SCALE QUANTIZATION (MAIN CONTROLS SLIDERS)
// ============================================================================
//
// Sister scenario to the display / playback tests above. This one targets the
// MainControlsComponent sliders (NoteSlider + OctaveSlider) under a scale-only
// change:
//   - the slider option sets are filtered to the in-scale pitch classes and
//     the reachable octaves,
//   - selecting a step shows the displayed (quantized) note in the sliders,
//   - moving the NoteSlider writes a new raw note through the non-destructive
//     path preserving the octave,
//   - moving the OctaveSlider writes a new raw note preserving the pitch
//     class,
//   - the value reported by NoteControls.getMidiNote() is consistent with the
//     same step's displayed value on the other surfaces.

SCENARIO("Non-destructive scale quantization drives MainControls sliders",
         "[user-story][scale][quantization][main-controls]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A paused app with raw step notes covering quantization boundary cases") {
        TestApp app;
        auto& state = app.state();

        // Pause so the playhead doesn't drift under us.
        app.navBar().clickPlayPause();
        app.processMessages();

        // Same fixture as the display / playback scenarios.
        const int rawNotes[8]      = { 60, 61, 62, 63, 66, 68, 70, 71 };
        const int expectedQuant[8] = { 60, 60, 60, 64, 67, 67, 72, 72 };

        for (int i = 0; i < 8; ++i) {
            state.setStepNotesLength(i, 1);
            state.setStepNote(i, 0, rawNotes[i]);
        }
        app.processMessages();

        WHEN("The scale is reduced to {C, E, G} via the piano roll") {
            const int keysToDisable[] = { 1, 2, 3, 5, 6, 8, 9, 10, 11 };
            for (int k : keysToDisable)
                app.pianoRoll().toggleNote(k);
            app.processMessages();

            auto& noteControls = app.mainScreen().getMainControls().getNoteControls();
            auto& noteSlider   = noteControls.getNoteSlider();
            auto& octaveSlider = noteControls.getOctaveSlider();

            THEN("NoteSlider options filter to exactly the 3 in-scale pitch classes") {
                // Select a step so updateNoteControlOptions has run for a
                // concrete current-note context.
                app.sequencerSteps().clickStep(3);
                app.processMessages();

                // NotchedSlider range is [0, numOptions-1], so max+1 == count.
                int noteCount = (int) noteSlider.getSlider().getSlider().getMaximum() + 1;
                REQUIRE(noteCount == 3);

                // Walking the slider over each index should yield exactly
                // {C=0, E=4, G=7} as chromatic pitch classes.
                std::set<int> seenPcs;
                for (int idx = 0; idx < noteCount; ++idx) {
                    noteSlider.getSlider().getSlider().setValue(
                        (double) idx, juce::dontSendNotification);
                    seenPcs.insert(noteSlider.getChromaticPitchClass());
                }
                REQUIRE(seenPcs == std::set<int>{0, 4, 7});
            }

            AND_THEN("OctaveSlider options cover the full range (default scale range unchanged)") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();

                int octaveCount = (int) octaveSlider.getSlider().getSlider().getMaximum() + 1;
                REQUIRE(octaveCount == 10);
            }

            AND_THEN("Selecting step 3 (raw Eb4=63) shows the quantized E4 in the sliders") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();

                REQUIRE(noteControls.getMidiNote() == expectedQuant[3]); // 64
                REQUIRE(noteSlider.getChromaticPitchClass() == (expectedQuant[3] % 12));
                REQUIRE(octaveSlider.getActualOctaveIndex() == (expectedQuant[3] / 12));
            }

            AND_THEN("Selecting step 6 (raw Bb4=70) shows the cross-octave quantized C5 in the sliders") {
                app.sequencerSteps().clickStep(6);
                app.processMessages();

                REQUIRE(noteControls.getMidiNote() == expectedQuant[6]); // 72
                REQUIRE(noteSlider.getChromaticPitchClass() == (expectedQuant[6] % 12));
                REQUIRE(octaveSlider.getActualOctaveIndex() == (expectedQuant[6] / 12));
            }

            AND_THEN("Moving the NoteSlider on step 3 writes a new raw note preserving the displayed octave") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();

                // Displayed value is E5? No — raw 63 quantizes to 64 (E4),
                // so octave = 64/12 = 5 in MIDI octave terms. The handler
                // rebuilds newNote = currentOctave * 12 + pitchClass using
                // that displayed octave.
                int displayedOct = expectedQuant[3] / 12; // 5

                // User drags NoteSlider to G. In the filtered 3-option scale
                // {C=0, E=4, G=7}, G is slider index 2. Drive the underlying
                // juce::Slider with notification so the full callback chain
                // (NoteSlider -> NoteControls -> ApplicationComponent handler)
                // fires exactly like a real drag.
                noteSlider.getSlider().getSlider().setValue(2.0, juce::sendNotificationSync);
                app.processMessages();

                int expectedNewNote = displayedOct * 12 + 7; // 67

                // Raw storage updated (G is in scale, so quantize is a no-op).
                REQUIRE(state.getStepNote(3, 0) == expectedNewNote);
                // Displayed note for step 3 matches.
                REQUIRE(state.getSelectedNoteForStep(3) == expectedNewNote);
                // Slider state reflects the new note.
                REQUIRE(noteControls.getMidiNote() == expectedNewNote);
                REQUIRE(noteSlider.getChromaticPitchClass() == 7);
                REQUIRE(octaveSlider.getActualOctaveIndex() == displayedOct);

                // Other steps' raw storage untouched.
                for (int i = 0; i < 8; ++i) {
                    if (i == 3) continue;
                    REQUIRE(state.getStepNote(i, 0) == rawNotes[i]);
                }
            }

            AND_THEN("Moving the OctaveSlider on step 6 writes a new raw note preserving the displayed pitch class") {
                app.sequencerSteps().clickStep(6);
                app.processMessages();

                // Displayed: raw 70 quantizes to 72 (C5), so pc=0.
                int displayedPc = expectedQuant[6] % 12; // 0

                // User drags OctaveSlider to octave 7. All 10 octaves are
                // present so the slider index matches the actual octave.
                octaveSlider.getSlider().getSlider().setValue(7.0, juce::sendNotificationSync);
                app.processMessages();

                int expectedNewNote = 7 * 12 + displayedPc; // 84

                REQUIRE(state.getStepNote(6, 0) == expectedNewNote);
                REQUIRE(state.getSelectedNoteForStep(6) == expectedNewNote);
                REQUIRE(noteControls.getMidiNote() == expectedNewNote);
                REQUIRE(noteSlider.getChromaticPitchClass() == displayedPc);
                REQUIRE(octaveSlider.getActualOctaveIndex() == 7);

                for (int i = 0; i < 8; ++i) {
                    if (i == 6) continue;
                    REQUIRE(state.getStepNote(i, 0) == rawNotes[i]);
                }
            }

            AND_THEN("NoteControls.getMidiNote matches the SequenceGrid-displayed note for the same step") {
                auto& grid = app.mainScreen().getSequenceOverview().getSequenceGrid();

                for (int stepIdx : { 3, 4, 6, 7 }) {
                    app.sequencerSteps().clickStep(stepIdx);
                    app.processMessages();

                    int shown = noteControls.getMidiNote();
                    REQUIRE(shown == expectedQuant[stepIdx]);

                    auto gridColour = grid.getSquareState(0, stepIdx).color;
                    auto expectedColour = state.getNoteColour(expectedQuant[stepIdx] % 12);
                    REQUIRE(gridColour == expectedColour);
                }
            }
        }
    }
}

// ============================================================================
// SCALE + RANGE QUANTIZATION TEST HELPERS
// ============================================================================
//
// Shared setup and verification functions used by the parameterised
// scale/range integration scenarios below. Keeps the individual SCANARIOs
// concise while the helpers encode the fixture, expected values, and
// assertion logic in one place.

namespace SRT { // Scale-Range Test helpers

// ---- Fixture data ----
constexpr int RAW[8]  = { 60, 61, 62, 63, 66, 68, 70, 71 };
constexpr int SUBS[4] = { 60, 61, 63, 66 };

// Expected under scale {C, E, G} — works for default range or [50, 109]
// (all raws land inside the window, so clamping is a no-op).
constexpr int CEG[8]      = { 60, 60, 60, 64, 67, 67, 72, 72 };
constexpr int CEG_SUBS[4] = { 60, 60, 64, 67 };

// Expected under scale {C, F} with range [50, 71]
// (clamp-then-snap path in ScaleQuantizer).
constexpr int CF_50_71[8] = { 60, 60, 60, 65, 65, 65, 65, 65 };

// ---- Setup helpers ----

inline void writeFixture(SequenceState& state) {
    for (int i = 0; i < 8; ++i) {
        state.setStepNotesLength(i, 1);
        state.setStepNote(i, 0, RAW[i]);
    }
}

inline void writeFixtureWithSubsteps(SequenceState& state) {
    writeFixture(state);
    state.setStepNotesLength(0, 4);
    for (int s = 0; s < 4; ++s)
        state.setStepNote(0, s, SUBS[s]);
}

inline void setupMidiMappings(TestApp& app) {
    // Enable latched scale mode so MIDI note-on toggles scale notes
    // (momentary mode uses note-on = enable, note-off = disable).
    if (auto* p = app.processor().getAPVTS().getParameter(Params::Keys::ID::SCALE_LATCH))
        p->setValueNotifyingHost(1.0f);

    for (int i = 0; i < 12; ++i)
        app.mapMidiNote(static_cast<MidiControlId>(
            static_cast<int>(MidiControlId::SCALE_NOTE_C) + i), 48 + i);
    app.mapMidiCC(MidiControlId::NOTE_RANGE_MIN, 50);
    app.mapMidiCC(MidiControlId::NOTE_RANGE_MAX, 51);
    app.mapMidiCC(MidiControlId::NOTE_SLIDER, 40);
    app.mapMidiCC(MidiControlId::OCTAVE_SLIDER, 41);
}

inline void setScaleCEG(TestApp& app, bool midi) {
    const int off[] = { 1, 2, 3, 5, 6, 8, 9, 10, 11 };
    for (int k : off) {
        if (midi) app.sendMidiNoteOn(48 + k);
        else      app.pianoRoll().toggleNote(k);
    }
}

inline void shiftScaleToCF(TestApp& app, bool midi) {
    auto toggle = [&](int k) {
        if (midi) app.sendMidiNoteOn(48 + k);
        else      app.pianoRoll().toggleNote(k);
    };
    toggle(5); // enable F
    toggle(4); // disable E
    toggle(7); // disable G
}

inline void setRange(TestApp& app, int lo, int hi, bool midi) {
    int size   = Scale::computeSizeFromRange(lo, hi);
    int offset = Scale::computeOffsetFromRange(lo, hi);
    if (midi) {
        app.sendMidiCC(50, offset);      // NOTE_RANGE_MIN -> offset
        app.sendMidiCC(51, size - 1);    // NOTE_RANGE_MAX -> value+1 = size
    } else {
        auto& apvts = app.processor().getAPVTS();
        if (auto* p = apvts.getParameter(Params::Keys::ID::SCALE_WINDOW_SIZE))
            p->setValueNotifyingHost(p->convertTo0to1((float)size));
        if (auto* p = apvts.getParameter(Params::Keys::ID::SCALE_WINDOW_OFFSET))
            p->setValueNotifyingHost(p->convertTo0to1((float)offset));
    }
}

inline void enableDestructive(TestApp& app, bool scale, bool range) {
    // Same code path as DestructiveToggleButton::mouseDown → setParamOn.
    // The APVTS parameterChanged callback in PluginProcessor triggers
    // bakeAllExistingNotes() on the OFF→ON transition.
    auto& apvts = app.processor().getAPVTS();
    if (scale)
        if (auto* p = apvts.getParameter(Params::Keys::ID::PREF_DESTRUCTIVE_SCALE))
            p->setValueNotifyingHost(1.0f);
    if (range)
        if (auto* p = apvts.getParameter(Params::Keys::ID::PREF_DESTRUCTIVE_RANGE))
            p->setValueNotifyingHost(1.0f);
}

// ---- Verification helpers ----

inline void checkScaleCEG(SequenceState& state) {
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

inline void checkScaleCF(SequenceState& state) {
    REQUIRE(state.getGlobalScaleNote(0));         // C
    REQUIRE(state.getGlobalScaleNote(5));         // F
    REQUIRE_FALSE(state.getGlobalScaleNote(4));   // E
    REQUIRE_FALSE(state.getGlobalScaleNote(7));   // G
}

inline void checkRaw(SequenceState& state, const int expected[], int count) {
    for (int i = 0; i < count; ++i) {
        INFO("step=" << i);
        REQUIRE(state.getStepNote(i, 0) == expected[i]);
    }
}

inline void checkQuantized(SequenceState& state, const int expected[], int count) {
    for (int i = 0; i < count; ++i) {
        INFO("step=" << i);
        REQUIRE(state.getCurrentNoteForStep(i) == expected[i]);
    }
}

inline void checkGrid(TestApp& app, const int expected[], int count) {
    auto& state = app.state();
    auto& grid  = app.mainScreen().getSequenceOverview().getSequenceGrid();
    for (int i = 0; i < count; ++i) {
        auto actual   = grid.getSquareState(0, i).color;
        auto expected_ = state.getNoteColour(expected[i] % 12);
        INFO("step=" << i);
        REQUIRE(actual == expected_);
    }
}

inline void checkNoteSliderPcs(TestApp& app, const std::set<int>& expectedPcs) {
    auto& ns = app.mainScreen().getMainControls().getNoteControls().getNoteSlider();
    int count = (int)ns.getSlider().getSlider().getMaximum() + 1;
    REQUIRE(count == (int)expectedPcs.size());
    std::set<int> pcs;
    for (int idx = 0; idx < count; ++idx) {
        ns.getSlider().getSlider().setValue((double)idx, juce::dontSendNotification);
        pcs.insert(ns.getChromaticPitchClass());
    }
    REQUIRE(pcs == expectedPcs);
}

inline void checkOctaveCount(TestApp& app, int expectedCount) {
    auto& os = app.mainScreen().getMainControls().getNoteControls().getOctaveSlider();
    REQUIRE((int)os.getSlider().getSlider().getMaximum() + 1 == expectedCount);
}

inline void checkSlider(TestApp& app, int expectedNote) {
    auto& nc = app.mainScreen().getMainControls().getNoteControls();
    REQUIRE(nc.getMidiNote() == expectedNote);
}

inline void checkPlayback(SequenceState& state, const int expected[], int count) {
    state.setCurrentIndex(-1);
    for (int i = 0; i < count; ++i) {
        int played = state.tick();
        INFO("step=" << i);
        REQUIRE(played == expected[i]);
    }
}

inline void checkActiveDot(TestApp& app, const int expected[], int count) {
    auto& state = app.state();
    auto& pr    = app.mainScreen().getPianoRoll();
    for (int i = 0; i < count; ++i) {
        state.setCurrentIndex(i);
        if (app.processor().onSequenceTick)
            app.processor().onSequenceTick();
        app.processMessages();
        INFO("step=" << i);
        REQUIRE(pr.getActiveNote() == (expected[i] % 12));
    }
}

} // namespace SRT

// ============================================================================
// NON-DESTRUCTIVE SCALE + RANGE QUANTIZATION (INTEGRATION)
// ============================================================================
//
// Capstone scenario for the scale-quantization cluster. Exercises scale AND
// range together, verifies every display surface, playback, active-note dot,
// and slider behaviour. Then SHIFTS both scale and range to a narrower
// window, forcing the clamp-then-snap path, and re-verifies.

SCENARIO("Non-destructive scale + range quantization across all surfaces and shifts",
         "[user-story][scale][range][quantization][integration]") {
    GamblerTestInitialiser juceInit;
    using namespace SRT;

    GIVEN("A paused app with raw fixture, range [50,109], scale {C, E, G}") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        writeFixture(state);
        setRange(app, 50, 109, false);
        setScaleCEG(app, false);
        app.processMessages();

        REQUIRE(state.getScaleRangeMin() == 50);
        REQUIRE(state.getScaleRangeMax() == 109);
        checkScaleCEG(state);

        WHEN("No shift — initial range + scale quantization") {
            THEN("Quantized getters match") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows quantized colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("NoteSlider lists {C, E, G}") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();
                checkNoteSliderPcs(app, {0, 4, 7});
            }
            AND_THEN("OctaveSlider shows 5 octaves for pc=E under [50,109]") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();
                checkOctaveCount(app, 5);
            }
            AND_THEN("NoteControls.getMidiNote matches") {
                for (int s : { 3, 6 }) {
                    app.sequencerSteps().clickStep(s);
                    app.processMessages();
                    checkSlider(app, CEG[s]);
                }
            }
            AND_THEN("state.tick() plays expected") {
                checkPlayback(state, CEG, 8);
            }
        }

        WHEN("User shifts scale to {C, F} and range to [50, 71]") {
            shiftScaleToCF(app, false);
            setRange(app, 50, 71, false);
            app.processMessages();

            REQUIRE(state.getScaleRangeMin() == 50);
            REQUIRE(state.getScaleRangeMax() == 71);
            checkScaleCF(state);

            THEN("Raw storage is UNCHANGED") {
                checkRaw(state, RAW, 8);
            }
            AND_THEN("Quantized getters match shifted expected") {
                checkQuantized(state, CF_50_71, 8);
            }
            AND_THEN("SequenceGrid refreshes") {
                checkGrid(app, CF_50_71, 8);
            }
            AND_THEN("NoteSlider lists {C, F}") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();
                checkNoteSliderPcs(app, {0, 5});
            }
            AND_THEN("OctaveSlider constrained to 2 octaves") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();
                checkSlider(app, CF_50_71[3]);
                checkOctaveCount(app, 2);
            }
            AND_THEN("state.tick() plays shifted expected") {
                checkPlayback(state, CF_50_71, 8);
            }
            AND_THEN("Active-note dot follows shifted expected") {
                checkActiveDot(app, CF_50_71, 8);
            }
            AND_THEN("Dragging NoteSlider writes correct raw note") {
                app.sequencerSteps().clickStep(0);
                app.processMessages();

                auto& noteSlider = app.mainScreen().getMainControls().getNoteControls().getNoteSlider();
                noteSlider.getSlider().getSlider().setValue(1.0, juce::sendNotificationSync);
                app.processMessages();

                REQUIRE(state.getStepNote(0, 0) == 65);
                REQUIRE(state.getSelectedNoteForStep(0) == 65);
                checkSlider(app, 65);
                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getStepNote(i, 0) == RAW[i]);
            }
        }
    }
}

// ============================================================================
// MIDI-DRIVEN SCALE QUANTIZATION (mirrors the UI-driven scale stories above)
// ============================================================================
//
// Same fixture and expected outputs as the UI-driven display / playback /
// main-controls stories, but every MIDI-mappable interaction is driven through
// mapped MIDI messages processed via processBlock instead of direct UI calls.
//
// MIDI-mappable controls exercised here:
//   SCALE_NOTE_C..B  — MIDI note-on toggles the scale note (12 mappings)
//   NOTE_SLIDER      — CC value -> chromatic pitch class 0-11
//   OCTAVE_SLIDER    — CC value -> octave 0-9
//
// Non-MIDI controls (step clicking, message pumping) still use the UI helpers.

SCENARIO("MIDI-driven scale quantization updates display surfaces and drives sliders",
         "[user-story][scale][quantization][midi-mapped]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A paused app with the standard 8-step fixture, scale notes mapped to MIDI notes 48-59") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        // Same fixture as the UI-driven scenarios.
        const int rawNotes[8]      = { 60, 61, 62, 63, 66, 68, 70, 71 };
        const int expectedQuant[8] = { 60, 60, 60, 64, 67, 67, 72, 72 };

        for (int i = 0; i < 8; ++i) {
            state.setStepNotesLength(i, 1);
            state.setStepNote(i, 0, rawNotes[i]);
        }

        // Step 0 gets 4 substeps for subsequencer bar verification.
        state.setStepNotesLength(0, 4);
        const int step0RawSubs[4]      = { 60, 61, 63, 66 };
        const int step0ExpectedSubs[4] = { 60, 60, 64, 67 };
        for (int s = 0; s < 4; ++s)
            state.setStepNote(0, s, step0RawSubs[s]);

        // Enable latched scale mode so MIDI note-on toggles scale notes.
        if (auto* p = app.processor().getAPVTS().getParameter(Params::Keys::ID::SCALE_LATCH))
            p->setValueNotifyingHost(1.0f);

        // Map all 12 scale notes to MIDI notes 48..59.
        for (int i = 0; i < 12; ++i)
            app.mapMidiNote(static_cast<MidiControlId>(
                static_cast<int>(MidiControlId::SCALE_NOTE_C) + i), 48 + i);

        // Map sliders to CCs for the driving portion.
        app.mapMidiCC(MidiControlId::NOTE_SLIDER, 40);
        app.mapMidiCC(MidiControlId::OCTAVE_SLIDER, 41);

        app.processMessages();

        WHEN("User toggles off 9 scale notes via MIDI note-ons, leaving {C, E, G}") {
            // Latched mode: MIDI note-on toggles scale notes off.
            const int keysToDisable[] = { 1, 2, 3, 5, 6, 8, 9, 10, 11 };
            for (int k : keysToDisable)
                app.sendMidiNoteOn(48 + k);
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
                REQUIRE(state.getCurrentNoteForStep(0) == step0ExpectedSubs[0]);
                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getCurrentNoteForStep(i) == expectedQuant[i]);
                for (int s = 0; s < 4; ++s)
                    REQUIRE(state.getQuantizedStepNote(0, s) == step0ExpectedSubs[s]);
            }

            AND_THEN("SequenceGrid shows quantized colours") {
                auto& grid = app.mainScreen().getSequenceOverview().getSequenceGrid();
                for (int i = 0; i < 8; ++i) {
                    int expected = (i == 0) ? step0ExpectedSubs[0] : expectedQuant[i];
                    auto actualColour = grid.getSquareState(0, i).color;
                    auto expectedColour = state.getNoteColour(expected % 12);
                    REQUIRE(actualColour == expectedColour);
                }
            }

            AND_THEN("Subsequencer bars for step 0 show quantized colours") {
                int activeSubstep = state.getStepNotesCurrentIndex(0);
                if (activeSubstep < 0) activeSubstep = 0;
                int checked = 0;
                auto subs = app.subsequenceControls();
                for (int s = 0; s < 4; ++s) {
                    if (s == activeSubstep) continue;
                    auto actualColour = subs.barColor(s);
                    auto expectedColour = state.getNoteColour(step0ExpectedSubs[s] % 12);
                    REQUIRE(actualColour == expectedColour);
                    ++checked;
                }
                REQUIRE(checked >= 3);
            }

            AND_THEN("NoteSlider filters to 3 options and sliders show correct quantized values") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();

                auto& noteControls = app.mainScreen().getMainControls().getNoteControls();
                auto& noteSlider   = noteControls.getNoteSlider();
                auto& octaveSlider = noteControls.getOctaveSlider();

                int noteCount = (int) noteSlider.getSlider().getSlider().getMaximum() + 1;
                REQUIRE(noteCount == 3);

                // Step 3: raw Eb4=63 -> quantized E4=64.
                REQUIRE(noteControls.getMidiNote() == expectedQuant[3]);
                REQUIRE(noteSlider.getChromaticPitchClass() == (expectedQuant[3] % 12));
                REQUIRE(octaveSlider.getActualOctaveIndex() == (expectedQuant[3] / 12));
            }

            AND_THEN("state.tick() plays the quantized values") {
                state.setCurrentIndex(-1);
                for (int i = 0; i < 8; ++i) {
                    int played = state.tick();
                    INFO("step=" << i);
                    REQUIRE(played == expectedQuant[i]);
                }
            }

            AND_THEN("The piano-roll active-note dot follows quantized pitch class") {
                auto& pianoRoll = app.mainScreen().getPianoRoll();
                for (int i = 0; i < 8; ++i) {
                    state.setCurrentIndex(i);
                    if (app.processor().onSequenceTick)
                        app.processor().onSequenceTick();
                    app.processMessages();

                    REQUIRE(pianoRoll.getActiveNote() == (expectedQuant[i] % 12));
                }
            }

            AND_THEN("MIDI CC on NOTE_SLIDER changes pitch class, preserving displayed octave") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();

                // Step 3 displays E4=64 (quantized). Octave = 64/12 = 5.
                // Send CC 40 with value 81 -> floor(81/127*11) = floor(7.02) = 7 (G).
                // Expected: 5*12 + 7 = 67.
                app.sendMidiCC(40, 81);
                app.processMessages();

                REQUIRE(state.getStepNote(3, 0) == 67);
                REQUIRE(state.getSelectedNoteForStep(3) == 67);

                // Other steps untouched.
                for (int i = 0; i < 8; ++i) {
                    if (i == 3) continue;
                    REQUIRE(state.getStepNote(i, 0) == rawNotes[i]);
                }
            }

            AND_THEN("MIDI CC on OCTAVE_SLIDER changes octave, preserving displayed pitch class") {
                app.sequencerSteps().clickStep(6);
                app.processMessages();

                // Step 6 displays C5=72 (quantized). pc = 0.
                // Send CC 41 with value 99 -> floor(99/127*9) = floor(7.02) = 7.
                // Expected: 7*12 + 0 = 84.
                app.sendMidiCC(41, 99);
                app.processMessages();

                REQUIRE(state.getStepNote(6, 0) == 84);
                REQUIRE(state.getSelectedNoteForStep(6) == 84);

                for (int i = 0; i < 8; ++i) {
                    if (i == 6) continue;
                    REQUIRE(state.getStepNote(i, 0) == rawNotes[i]);
                }
            }
        }
    }
}

// ============================================================================
// MIDI-DRIVEN SCALE LATCH / MOMENTARY MODE
// ============================================================================
//
// Verifies that MIDI-mapped scale note messages honour the SCALE_LATCH param:
//   - Latched (lock ON):   note-on toggles, note-off is ignored
//   - Momentary (lock OFF): note-on turns scale note ON, note-off turns it OFF

SCENARIO("MIDI-mapped scale notes honour scale latch vs momentary mode",
         "[user-story][scale][midi-mapped][latch]") {
    GamblerTestInitialiser juceInit;

    GIVEN("An app with scale notes mapped to MIDI notes 48-59") {
        TestApp app;
        auto& state = app.state();
        auto& apvts = app.processor().getAPVTS();

        // Map all 12 scale notes to MIDI notes 48..59.
        for (int i = 0; i < 12; ++i)
            app.mapMidiNote(static_cast<MidiControlId>(
                static_cast<int>(MidiControlId::SCALE_NOTE_C) + i), 48 + i);
        app.processMessages();

        // Confirm all 12 notes start enabled (chromatic).
        for (int i = 0; i < 12; ++i)
            REQUIRE(state.getGlobalScaleNote(i));

        WHEN("Scale latch is ON (latched/toggle mode)") {
            apvts.getParameter(Params::Keys::ID::SCALE_LATCH)
                ->setValueNotifyingHost(1.0f);
            app.processMessages();

            THEN("Note-on toggles a scale note OFF, note-off is ignored") {
                // Toggle D (index 2) off with note-on
                app.sendMidiNoteOn(50);  // MIDI note 50 = D
                app.processMessages();
                REQUIRE_FALSE(state.getGlobalScaleNote(2));

                // Note-off should NOT toggle it back on
                app.sendMidiNoteOff(50);
                app.processMessages();
                REQUIRE_FALSE(state.getGlobalScaleNote(2));

                // Another note-on toggles it back ON
                app.sendMidiNoteOn(50);
                app.processMessages();
                REQUIRE(state.getGlobalScaleNote(2));
            }
        }

        WHEN("Scale latch is OFF (momentary mode)") {
            apvts.getParameter(Params::Keys::ID::SCALE_LATCH)
                ->setValueNotifyingHost(0.0f);
            app.processMessages();

            THEN("Note-on keeps scale note ON, note-off turns it OFF") {
                // All notes start ON. Send note-on for D — should stay ON (already on).
                app.sendMidiNoteOn(50);
                app.processMessages();
                REQUIRE(state.getGlobalScaleNote(2));

                // Note-off turns D OFF (momentary release)
                app.sendMidiNoteOff(50);
                app.processMessages();
                REQUIRE_FALSE(state.getGlobalScaleNote(2));

                // Note-on turns it back ON
                app.sendMidiNoteOn(50);
                app.processMessages();
                REQUIRE(state.getGlobalScaleNote(2));
            }

            AND_THEN("Multiple keys held then released independently") {
                // Hold D and F via note-on — both stay ON
                app.sendMidiNoteOn(50);  // D
                app.sendMidiNoteOn(53);  // F
                app.processMessages();
                REQUIRE(state.getGlobalScaleNote(2));  // D
                REQUIRE(state.getGlobalScaleNote(5));  // F

                // Release D — D goes OFF, F stays ON
                app.sendMidiNoteOff(50);
                app.processMessages();
                REQUIRE_FALSE(state.getGlobalScaleNote(2));  // D off
                REQUIRE(state.getGlobalScaleNote(5));         // F still on

                // Release F — F goes OFF
                app.sendMidiNoteOff(53);
                app.processMessages();
                REQUIRE_FALSE(state.getGlobalScaleNote(5));  // F off
            }
        }
    }
}

// ============================================================================
// MIDI-DRIVEN SCALE + RANGE QUANTIZATION (mirrors the UI-driven integration)
// ============================================================================

SCENARIO("MIDI-driven scale + range quantization across all surfaces and shifts",
         "[user-story][scale][range][quantization][midi-mapped]") {
    GamblerTestInitialiser juceInit;
    using namespace SRT;

    GIVEN("A paused app with raw fixture, MIDI mappings for scale/range/sliders") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        writeFixture(state);
        setupMidiMappings(app);
        setRange(app, 50, 109, true);
        setScaleCEG(app, true);
        app.processMessages();

        REQUIRE(state.getScaleRangeMin() == 50);
        REQUIRE(state.getScaleRangeMax() == 109);
        checkScaleCEG(state);

        WHEN("No shift — initial MIDI-set range + scale quantization") {
            THEN("Quantized getters match") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows quantized colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("NoteSlider lists {C, E, G} and OctaveSlider shows 5 octaves") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();
                checkNoteSliderPcs(app, {0, 4, 7});
                checkOctaveCount(app, 5);
            }
            AND_THEN("NoteControls.getMidiNote matches") {
                for (int s : { 3, 6 }) {
                    app.sequencerSteps().clickStep(s);
                    app.processMessages();
                    checkSlider(app, CEG[s]);
                }
            }
            AND_THEN("state.tick() plays expected") {
                checkPlayback(state, CEG, 8);
            }
        }

        WHEN("User shifts scale to {C, F} and range to [50, 71] via MIDI") {
            shiftScaleToCF(app, true);
            setRange(app, 50, 71, true);
            app.processMessages();

            REQUIRE(state.getScaleRangeMin() == 50);
            REQUIRE(state.getScaleRangeMax() == 71);
            checkScaleCF(state);

            THEN("Raw storage is UNCHANGED") {
                checkRaw(state, RAW, 8);
            }
            AND_THEN("Quantized getters match shifted expected") {
                checkQuantized(state, CF_50_71, 8);
            }
            AND_THEN("SequenceGrid refreshes") {
                checkGrid(app, CF_50_71, 8);
            }
            AND_THEN("NoteSlider lists {C, F} and OctaveSlider constrained to 2") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();
                checkNoteSliderPcs(app, {0, 5});
                checkSlider(app, CF_50_71[3]);
                checkOctaveCount(app, 2);
            }
            AND_THEN("state.tick() plays shifted expected") {
                checkPlayback(state, CF_50_71, 8);
            }
            AND_THEN("Active-note dot follows shifted expected") {
                checkActiveDot(app, CF_50_71, 8);
            }
            AND_THEN("MIDI CC on NOTE_SLIDER changes pitch class to F on step 0") {
                app.sequencerSteps().clickStep(0);
                app.processMessages();

                // Step 0 displays C5=60. Octave = 5.
                // CC 40 value 58 -> floor(58/127*11) = 5 (F). newNote = 5*12+5 = 65.
                app.sendMidiCC(40, 58);
                app.processMessages();

                REQUIRE(state.getStepNote(0, 0) == 65);
                REQUIRE(state.getSelectedNoteForStep(0) == 65);
                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getStepNote(i, 0) == RAW[i]);
            }
            AND_THEN("MIDI CC on OCTAVE_SLIDER changes octave on step 3") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();

                // Step 3 displays F4=65. pc=5.
                // CC 41 value 57 -> floor(57/127*9) = 4. newNote = 4*12+5 = 53.
                app.sendMidiCC(41, 57);
                app.processMessages();

                REQUIRE(state.getStepNote(3, 0) == 53);
                REQUIRE(state.getSelectedNoteForStep(3) == 53);
                for (int i = 0; i < 8; ++i) {
                    if (i == 3) continue;
                    REQUIRE(state.getStepNote(i, 0) == RAW[i]);
                }
            }
        }
    }
}

// ============================================================================
// DESTRUCTIVE SCALE QUANTIZATION (UI-driven)
// ============================================================================
//
// Same fixture and scale as the non-destructive tests, but destructive-scale
// mode is enabled AFTER setting the scale. This causes bakeAllExistingNotes()
// to rewrite raw storage, so raw == quantized. After that, changing the scale
// does NOT retroactively update existing notes — only new writes go through
// bakeNoteForStorage().

SCENARIO("Destructive scale quantization bakes notes and freezes on scale shift",
         "[user-story][scale][quantization][destructive]") {
    GamblerTestInitialiser juceInit;
    using namespace SRT;

    GIVEN("A paused app with raw fixture, range [50,109], scale {C, E, G}") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        writeFixture(state);
        setRange(app, 50, 109, false);
        setScaleCEG(app, false);
        app.processMessages();

        REQUIRE(state.getScaleRangeMin() == 50);
        REQUIRE(state.getScaleRangeMax() == 109);
        checkScaleCEG(state);

        WHEN("User enables destructive scale mode") {
            enableDestructive(app, true, false);
            app.processMessages();

            THEN("Raw storage is now BAKED to the CEG-quantized values") {
                checkRaw(state, CEG, 8);
            }
            AND_THEN("Quantized getters match (same values, now from baked raw)") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows quantized colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("NoteSlider lists {C, E, G}") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();
                checkNoteSliderPcs(app, {0, 4, 7});
            }
            AND_THEN("OctaveSlider shows 5 octaves for pc=E under [50,109]") {
                app.sequencerSteps().clickStep(3);
                app.processMessages();
                checkOctaveCount(app, 5);
            }
            AND_THEN("NoteControls.getMidiNote matches") {
                for (int s : { 3, 6 }) {
                    app.sequencerSteps().clickStep(s);
                    app.processMessages();
                    checkSlider(app, CEG[s]);
                }
            }
            AND_THEN("state.tick() plays the baked values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("Active-note dot follows baked values") {
                checkActiveDot(app, CEG, 8);
            }
        }

        WHEN("User enables destructive scale, then shifts scale to {C, F}") {
            enableDestructive(app, true, false);
            app.processMessages();

            // Notes are now baked to CEG: { 60, 60, 60, 64, 67, 67, 72, 72 }
            // Now shift scale to {C, F} — existing notes should NOT change.
            shiftScaleToCF(app, false);
            app.processMessages();

            checkScaleCF(state);

            // In destructive-scale mode, quantize() skips scale, so the
            // baked-to-CEG values display as-is even though E and G are no
            // longer in the active scale.
            THEN("Raw storage still holds the CEG-baked values (NOT re-baked to CF)") {
                checkRaw(state, CEG, 8);
            }
            AND_THEN("Quantized getters return the CEG-baked values unchanged") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows the CEG-baked colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("state.tick() plays the CEG-baked values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("Dragging NoteSlider writes a baked note through the new scale") {
                app.sequencerSteps().clickStep(0);
                app.processMessages();

                auto& noteSlider = app.mainScreen().getMainControls().getNoteControls().getNoteSlider();

                // Drag to F (index 1 in {C, F}).
                noteSlider.getSlider().getSlider().setValue(1.0, juce::sendNotificationSync);
                app.processMessages();

                // bakeNoteForStorage with destructiveScale ON bakes to the
                // current scale {C, F}. newNote = 5*12+5 = 65 (F5), which is
                // in scale, so bake returns 65.
                REQUIRE(state.getStepNote(0, 0) == 65);
                REQUIRE(state.getSelectedNoteForStep(0) == 65);
                checkSlider(app, 65);

                // Other steps still hold CEG-baked values.
                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getStepNote(i, 0) == CEG[i]);
            }
        }
    }
}

// ============================================================================
// DESTRUCTIVE SCALE QUANTIZATION (MIDI-driven)
// ============================================================================

SCENARIO("MIDI-driven destructive scale quantization bakes notes and freezes on scale shift",
         "[user-story][scale][quantization][destructive][midi-mapped]") {
    GamblerTestInitialiser juceInit;
    using namespace SRT;

    GIVEN("A paused app with raw fixture, MIDI mappings, range [50,109], scale {C, E, G}") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        writeFixture(state);
        setupMidiMappings(app);
        setRange(app, 50, 109, true);
        setScaleCEG(app, true);
        app.processMessages();

        REQUIRE(state.getScaleRangeMin() == 50);
        REQUIRE(state.getScaleRangeMax() == 109);
        checkScaleCEG(state);

        WHEN("User enables destructive scale mode") {
            enableDestructive(app, true, false);
            app.processMessages();

            THEN("Raw storage is BAKED to CEG-quantized values") {
                checkRaw(state, CEG, 8);
            }
            AND_THEN("Quantized getters match") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows quantized colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("state.tick() plays baked values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("Active-note dot follows baked values") {
                checkActiveDot(app, CEG, 8);
            }
        }

        WHEN("User enables destructive scale, then shifts scale to {C, F} via MIDI") {
            enableDestructive(app, true, false);
            app.processMessages();

            shiftScaleToCF(app, true);
            app.processMessages();
            checkScaleCF(state);

            THEN("Raw storage still holds CEG-baked values") {
                checkRaw(state, CEG, 8);
            }
            AND_THEN("Quantized getters return CEG-baked values unchanged") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows CEG-baked colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("state.tick() plays CEG-baked values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("MIDI CC on NOTE_SLIDER writes a baked note through the new scale") {
                app.sequencerSteps().clickStep(0);
                app.processMessages();

                // Step 0 = C5=60. CC 40 value 58 -> pc=5 (F).
                // newNote = 5*12+5 = 65. bake with {C,F} -> 65 (in scale).
                app.sendMidiCC(40, 58);
                app.processMessages();

                REQUIRE(state.getStepNote(0, 0) == 65);
                REQUIRE(state.getSelectedNoteForStep(0) == 65);
                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getStepNote(i, 0) == CEG[i]);
            }
        }
    }
}

// ============================================================================
// DESTRUCTIVE RANGE QUANTIZATION (UI-driven)
// ============================================================================
//
// Tests destructive-range mode with scale {C, E, G}. When range-destructive is
// enabled, bakeAllExistingNotes() clamps raw notes to the range (but does NOT
// apply scale since bakeS is false). After that, changing the range does NOT
// retroactively update existing notes.

SCENARIO("Destructive range quantization bakes notes and freezes on range shift",
         "[user-story][range][quantization][destructive]") {
    GamblerTestInitialiser juceInit;
    using namespace SRT;

    GIVEN("A paused app with raw fixture, range [50,109], scale {C, E, G}") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        writeFixture(state);
        setRange(app, 50, 109, false);
        setScaleCEG(app, false);
        app.processMessages();

        REQUIRE(state.getScaleRangeMin() == 50);
        REQUIRE(state.getScaleRangeMax() == 109);
        checkScaleCEG(state);

        WHEN("User enables destructive range mode") {
            enableDestructive(app, false, true);
            app.processMessages();

            // bake() with destructiveRange ON clamps to [50,109] — all raws
            // are already inside, so clamping is a no-op. bake() does NOT
            // apply scale when only destructiveRange is ON (bakeS is false).
            THEN("Raw storage is unchanged (all raws already inside [50,109])") {
                checkRaw(state, RAW, 8);
            }
            // quantize() skips range (destructiveRange=true), but still
            // applies scale (destructiveScale=false).
            AND_THEN("Quantized getters match CEG (scale still applied by display)") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows quantized colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("state.tick() plays scale-quantized values") {
                checkPlayback(state, CEG, 8);
            }
        }

        WHEN("User enables destructive range, then tightens range to [50, 71]") {
            enableDestructive(app, false, true);
            app.processMessages();

            checkRaw(state, RAW, 8);

            setRange(app, 50, 71, false);
            app.processMessages();

            REQUIRE(state.getScaleRangeMin() == 50);
            REQUIRE(state.getScaleRangeMax() == 71);

            // bakeAllExistingNotes only ran on the OFF->ON transition (with
            // old range [50,109]). Changing the range afterward does NOT
            // re-bake.
            THEN("Raw storage is still unchanged (bake only ran once on transition)") {
                checkRaw(state, RAW, 8);
            }
            // quantize() skips range (destructiveRange=true) but still
            // applies scale. Without range clamping, all raws map to
            // CEG-quantized values (same as full-range scale-only scenario).
            AND_THEN("Quantized getters match CEG (range skipped, scale applied)") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows CEG-quantized colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("state.tick() plays CEG-quantized values") {
                checkPlayback(state, CEG, 8);
            }
        }
    }
}

// ============================================================================
// DESTRUCTIVE RANGE QUANTIZATION (MIDI-driven)
// ============================================================================

SCENARIO("MIDI-driven destructive range quantization bakes notes and freezes on range shift",
         "[user-story][range][quantization][destructive][midi-mapped]") {
    GamblerTestInitialiser juceInit;
    using namespace SRT;

    GIVEN("A paused app with raw fixture, MIDI mappings, range [50,109], scale {C, E, G}") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        writeFixture(state);
        setupMidiMappings(app);
        setRange(app, 50, 109, true);
        setScaleCEG(app, true);
        app.processMessages();

        REQUIRE(state.getScaleRangeMin() == 50);
        REQUIRE(state.getScaleRangeMax() == 109);
        checkScaleCEG(state);

        WHEN("User enables destructive range mode") {
            enableDestructive(app, false, true);
            app.processMessages();

            THEN("Raw storage unchanged (all raws inside [50,109])") {
                checkRaw(state, RAW, 8);
            }
            AND_THEN("Quantized getters match CEG") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("state.tick() plays scale-quantized values") {
                checkPlayback(state, CEG, 8);
            }
        }

        WHEN("User enables destructive range, then tightens range to [50, 71] via MIDI") {
            enableDestructive(app, false, true);
            app.processMessages();
            checkRaw(state, RAW, 8);

            setRange(app, 50, 71, true);
            app.processMessages();

            REQUIRE(state.getScaleRangeMin() == 50);
            REQUIRE(state.getScaleRangeMax() == 71);

            THEN("Raw storage is still unchanged") {
                checkRaw(state, RAW, 8);
            }
            AND_THEN("Quantized getters match CEG (range skipped, scale applied)") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("state.tick() plays CEG-quantized values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("Active-note dot follows CEG-quantized values") {
                checkActiveDot(app, CEG, 8);
            }
        }
    }
}

// ============================================================================
// DESTRUCTIVE SCALE + RANGE COMBINED (UI-driven)
// ============================================================================
//
// Both destructive flags ON. bake() applies range clamp AND scale snap on
// writes, quantize() is a full passthrough. After enabling, changing scale
// and/or range does NOT retroactively update existing notes. New writes go
// through bake() which applies both current scale AND range.

SCENARIO("Destructive scale+range combined bakes notes and freezes on shift",
         "[user-story][scale][range][quantization][destructive]") {
    GamblerTestInitialiser juceInit;
    using namespace SRT;

    GIVEN("A paused app with raw fixture, range [50,109], scale {C, E, G}") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        writeFixture(state);
        setRange(app, 50, 109, false);
        setScaleCEG(app, false);
        app.processMessages();

        REQUIRE(state.getScaleRangeMin() == 50);
        REQUIRE(state.getScaleRangeMax() == 109);
        checkScaleCEG(state);

        WHEN("User enables both destructive scale and range") {
            enableDestructive(app, true, true);
            app.processMessages();

            // bake() clamps to [50,109] (no-op, all inside) then snaps to
            // scale {C,E,G}. Result is the same as CEG.
            THEN("Raw storage is BAKED to CEG-quantized values") {
                checkRaw(state, CEG, 8);
            }
            // quantize() skips both scale and range -> full passthrough.
            AND_THEN("Quantized getters match (passthrough from baked raw)") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows quantized colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("NoteControls.getMidiNote matches") {
                for (int s : { 3, 6 }) {
                    app.sequencerSteps().clickStep(s);
                    app.processMessages();
                    checkSlider(app, CEG[s]);
                }
            }
            AND_THEN("state.tick() plays the baked values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("Active-note dot follows baked values") {
                checkActiveDot(app, CEG, 8);
            }
        }

        WHEN("User enables both destructive modes, then shifts to {C,F} + [50,71]") {
            enableDestructive(app, true, true);
            app.processMessages();

            // Notes baked to CEG under [50,109].
            checkRaw(state, CEG, 8);

            shiftScaleToCF(app, false);
            setRange(app, 50, 71, false);
            app.processMessages();

            checkScaleCF(state);
            REQUIRE(state.getScaleRangeMin() == 50);
            REQUIRE(state.getScaleRangeMax() == 71);

            // quantize() is full passthrough, so existing baked notes display
            // as-is regardless of the new scale/range.
            THEN("Raw storage still holds CEG-baked values") {
                checkRaw(state, CEG, 8);
            }
            AND_THEN("Quantized getters return CEG-baked values (full passthrough)") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows CEG-baked colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("state.tick() plays CEG-baked values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("Dragging NoteSlider writes a note baked through BOTH new scale and range") {
                app.sequencerSteps().clickStep(0);
                app.processMessages();

                auto& noteSlider = app.mainScreen().getMainControls().getNoteControls().getNoteSlider();

                // Step 0 = C5=60. Drag to F (index 1 in {C,F}).
                // newNote = 5*12+5 = 65. bake clamps to [50,71] (65 inside),
                // then scale {C,F}: 65 is F, in scale. Result = 65.
                noteSlider.getSlider().getSlider().setValue(1.0, juce::sendNotificationSync);
                app.processMessages();

                REQUIRE(state.getStepNote(0, 0) == 65);
                REQUIRE(state.getSelectedNoteForStep(0) == 65);
                checkSlider(app, 65);

                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getStepNote(i, 0) == CEG[i]);
            }
            AND_THEN("Dragging OctaveSlider out of range gets clamped+snapped by bake") {
                // Use step 0 (C5=60, pc=0) — its baked pc is in the new
                // scale {C,F}, so the NoteSlider can display it cleanly.
                app.sequencerSteps().clickStep(0);
                app.processMessages();

                auto& octaveSlider = app.mainScreen().getMainControls().getNoteControls().getOctaveSlider();

                // Drive to highest available octave index.
                int maxIdx = (int) octaveSlider.getSlider().getSlider().getMaximum();
                octaveSlider.getSlider().getSlider().setValue((double) maxIdx, juce::sendNotificationSync);
                app.processMessages();

                // The written note must be inside [50,71] and in {C,F}.
                int written = state.getStepNote(0, 0);
                REQUIRE(written >= 50);
                REQUIRE(written <= 71);
                int pc = written % 12;
                REQUIRE((pc == 0 || pc == 5)); // C or F
            }
        }
    }
}

// ============================================================================
// DESTRUCTIVE SCALE + RANGE COMBINED (MIDI-driven)
// ============================================================================

SCENARIO("MIDI-driven destructive scale+range combined bakes notes and freezes on shift",
         "[user-story][scale][range][quantization][destructive][midi-mapped]") {
    GamblerTestInitialiser juceInit;
    using namespace SRT;

    GIVEN("A paused app with raw fixture, MIDI mappings, range [50,109], scale {C, E, G}") {
        TestApp app;
        auto& state = app.state();

        app.navBar().clickPlayPause();
        app.processMessages();

        writeFixture(state);
        setupMidiMappings(app);
        setRange(app, 50, 109, true);
        setScaleCEG(app, true);
        app.processMessages();

        REQUIRE(state.getScaleRangeMin() == 50);
        REQUIRE(state.getScaleRangeMax() == 109);
        checkScaleCEG(state);

        WHEN("User enables both destructive modes") {
            enableDestructive(app, true, true);
            app.processMessages();

            THEN("Raw storage is BAKED to CEG") {
                checkRaw(state, CEG, 8);
            }
            AND_THEN("Quantized getters match (full passthrough)") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows quantized colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("state.tick() plays baked values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("Active-note dot follows baked values") {
                checkActiveDot(app, CEG, 8);
            }
        }

        WHEN("User enables both destructive modes, then shifts to {C,F} + [50,71] via MIDI") {
            enableDestructive(app, true, true);
            app.processMessages();
            checkRaw(state, CEG, 8);

            shiftScaleToCF(app, true);
            setRange(app, 50, 71, true);
            app.processMessages();

            checkScaleCF(state);
            REQUIRE(state.getScaleRangeMin() == 50);
            REQUIRE(state.getScaleRangeMax() == 71);

            THEN("Raw storage still holds CEG-baked values") {
                checkRaw(state, CEG, 8);
            }
            AND_THEN("Quantized getters return CEG-baked values (full passthrough)") {
                checkQuantized(state, CEG, 8);
            }
            AND_THEN("SequenceGrid shows CEG-baked colours") {
                checkGrid(app, CEG, 8);
            }
            AND_THEN("state.tick() plays CEG-baked values") {
                checkPlayback(state, CEG, 8);
            }
            AND_THEN("Active-note dot follows CEG-baked values") {
                checkActiveDot(app, CEG, 8);
            }
            AND_THEN("MIDI CC on NOTE_SLIDER writes a note baked through both scale and range") {
                app.sequencerSteps().clickStep(0);
                app.processMessages();

                // Step 0 = C5=60. CC 40 value 58 -> pc=5 (F).
                // newNote = 5*12+5 = 65. bake: clamp [50,71] ok, {C,F} ok.
                app.sendMidiCC(40, 58);
                app.processMessages();

                REQUIRE(state.getStepNote(0, 0) == 65);
                REQUIRE(state.getSelectedNoteForStep(0) == 65);
                for (int i = 1; i < 8; ++i)
                    REQUIRE(state.getStepNote(i, 0) == CEG[i]);
            }
            AND_THEN("MIDI CC on OCTAVE_SLIDER out of range gets clamped+snapped") {
                app.sequencerSteps().clickStep(0);
                app.processMessages();

                // Step 0 = C5=60. CC 41 value 99 -> oct=7.
                // MIDI path: currentNote=60 (passthrough), pc=0.
                // newNote = 7*12+0 = 84. bake: clamp to [50,71] -> 71.
                // 71=B not in {C,F}. Search: 70 Bb no, 69 A no, 68 Ab no,
                // 67 G no, 66 Gb no, 65 F yes -> 65.
                app.sendMidiCC(41, 99);
                app.processMessages();

                REQUIRE(state.getStepNote(0, 0) == 65);
                REQUIRE(state.getSelectedNoteForStep(0) == 65);
            }
        }
    }
}

