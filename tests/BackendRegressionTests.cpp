/**
 * BackendRegressionTests.cpp - Unit-level regression tests for backend bugs
 *
 * Each scenario documents a bug found by auditing the backend for complexity
 * traps (index math, sample-window checks, negative-number arithmetic).
 * The GIVEN/WHEN text describes the failure mode the test guards against.
 */

#include "helpers/TestApp.h"
#include <catch2/catch_test_macros.hpp>
#include <backend/NoteOffScheduler.h>
#include <backend/Clock.h>
#include <backend/InternalState.h>
#include <data/Params.h>

// ============================================================================
// NoteOffScheduler: past-due scheduled note-offs must still fire
// ============================================================================
//
// processBlock handles scheduled note-offs at the TOP of the block, before
// tickSequencer schedules new ones. A note-off whose target sample lands
// inside the same block it was scheduled in is therefore always seen one
// block late — its sample position is already in the past. The old window
// check (scheduled >= blockStart) silently skipped it, so the note rang on
// until the next step's immediate note-off: short gate values turned into
// full legato whenever noteOffDelay < samples remaining in the block.

SCENARIO("NoteOffScheduler fires past-due scheduled note-offs at block start",
         "[regression][note-off]") {
    GIVEN("A scheduler with a note-off scheduled at sample 100") {
        NoteOffScheduler sched;
        sched.setLastPlayedNote(60);
        sched.scheduleNoteOff(60, 100);

        WHEN("The next block starts at sample 512 (the target is in the past)") {
            juce::MidiBuffer buf;
            bool emitted = sched.processScheduledNoteOff(buf, 1, nullptr, 512, 512);

            THEN("The note-off is emitted at offset 0 instead of being lost") {
                REQUIRE(emitted);
                REQUIRE(buf.getNumEvents() == 1);
                for (const auto& metadata : buf) {
                    auto msg = metadata.getMessage();
                    REQUIRE(msg.isNoteOff());
                    REQUIRE(msg.getNoteNumber() == 60);
                    REQUIRE(metadata.samplePosition == 0);
                }
            }

            AND_THEN("It does not fire a second time") {
                juce::MidiBuffer buf2;
                REQUIRE(sched.processScheduledNoteOff(buf2, 1, nullptr, 1024, 512) == false);
                REQUIRE(buf2.getNumEvents() == 0);
            }
        }
    }

    GIVEN("A scheduler with a note-off scheduled inside the current block") {
        NoteOffScheduler sched;
        sched.setLastPlayedNote(72);
        sched.scheduleNoteOff(72, 600);

        WHEN("The block [512, 1024) is processed") {
            juce::MidiBuffer buf;
            bool emitted = sched.processScheduledNoteOff(buf, 1, nullptr, 512, 512);

            THEN("The note-off fires at the exact sub-block offset") {
                REQUIRE(emitted);
                REQUIRE(buf.getNumEvents() == 1);
                for (const auto& metadata : buf) {
                    REQUIRE(metadata.getMessage().isNoteOff());
                    REQUIRE(metadata.samplePosition == 88);  // 600 - 512
                }
            }
        }
    }

    GIVEN("A scheduler with a note-off scheduled beyond the current block") {
        NoteOffScheduler sched;
        sched.setLastPlayedNote(72);
        sched.scheduleNoteOff(72, 2000);

        WHEN("The block [512, 1024) is processed") {
            juce::MidiBuffer buf;
            bool emitted = sched.processScheduledNoteOff(buf, 1, nullptr, 512, 512);

            THEN("Nothing fires yet — the note-off stays scheduled") {
                REQUIRE(emitted == false);
                REQUIRE(buf.getNumEvents() == 0);
            }
        }
    }
}

// ============================================================================
// Clock: accumulated MIDI clock pulses must not be silently capped
// ============================================================================
//
// accumulateMidiClockPulse() adds pulsesPerTick (6 at 16ths) per sequencer
// tick. When two ticks land in one audio block (large buffers / high BPM),
// 12 pulses accumulate — but getMidiClockPulsesToSend() capped the return at
// pulsesPerTick AND zeroed the accumulator, silently dropping the rest.
// External gear following our MIDI clock drifted behind by one tick each
// time this happened.

SCENARIO("Clock returns all accumulated MIDI clock pulses",
         "[regression][clock]") {
    GIVEN("An internal-mode clock at 16th-note subdivision") {
        Clock clock;  // defaults: INTERNAL, SIXTEENTH

        WHEN("Two sequencer ticks accumulate pulses in the same block") {
            clock.accumulateMidiClockPulse();
            clock.accumulateMidiClockPulse();

            THEN("All 12 pulses are reported, none dropped") {
                REQUIRE(clock.getMidiClockPulsesToSend() == 12);
            }

            AND_THEN("The accumulator is drained afterwards") {
                clock.getMidiClockPulsesToSend();
                REQUIRE(clock.getMidiClockPulsesToSend() == 0);
            }
        }
    }
}

// ============================================================================
// InternalState: substep count must never come back as 0
// ============================================================================
//
// Step::advanceSubstep computes (currentSubstep + 1) % substepCount on the
// audio thread. The setter clamps to [1, MAX_SUBSTEPS], but setState()
// swallows a whole ValueTree from XML — a corrupted or hand-edited preset
// with step_N_notes_len="0" would make the getter return 0 and the audio
// thread divide by zero.

SCENARIO("Corrupted preset data cannot produce a zero substep count",
         "[regression][internal-state]") {
    GIVEN("An InternalState loaded from a tree with a zero notes_len") {
        InternalState state;
        juce::ValueTree corrupted(InternalState::IDs::ROOT_TYPE);
        corrupted.setProperty(InternalState::IDs::stepNotesLength(0), 0, nullptr);
        corrupted.setProperty(InternalState::IDs::stepNotesLength(3), -5, nullptr);
        state.setState(corrupted);

        THEN("getStepNotesLength clamps to at least 1") {
            REQUIRE(state.getStepNotesLength(0) >= 1);
            REQUIRE(state.getStepNotesLength(3) >= 1);
        }
    }
}

// ============================================================================
// MIDI-mapped bake buttons: learned mappings must actually fire
// ============================================================================
//
// DESTRUCTIVE_SCALE and DESTRUCTIVE_RANGE are learnable in the UI (the bake
// buttons in the nav bar paint MIDI-learn overlays and register with the
// registry), but handleMidiMapping had no case for them — a learned mapping
// fell through to default: and silently did nothing.

SCENARIO("MIDI mapped to the bake buttons toggles destructive modes",
         "[regression][midi-routing][destructive]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A CC mapped to the Bake Scale button") {
        TestApp app;
        auto& state = app.state();
        app.mapMidiCC(MidiControlId::DESTRUCTIVE_SCALE, 21);
        REQUIRE(state.getDestructiveScale() == false);

        WHEN("The mapped CC arrives high") {
            app.sendMidiCC(21, 127);

            THEN("Destructive scale turns on") {
                REQUIRE(state.getDestructiveScale() == true);
            }

            AND_WHEN("The CC goes low (button released)") {
                app.sendMidiCC(21, 0);

                THEN("It stays on — press toggles, release is ignored") {
                    REQUIRE(state.getDestructiveScale() == true);
                }
            }

            AND_WHEN("The CC arrives high again") {
                app.sendMidiCC(21, 0);
                app.sendMidiCC(21, 127);

                THEN("Destructive scale toggles back off") {
                    REQUIRE(state.getDestructiveScale() == false);
                }
            }
        }
    }

    GIVEN("A note mapped to the Bake Range button") {
        TestApp app;
        auto& state = app.state();
        app.mapMidiNote(MidiControlId::DESTRUCTIVE_RANGE, 40);
        REQUIRE(state.getDestructiveRange() == false);

        WHEN("The mapped note is pressed") {
            app.sendMidiNoteOn(40);

            THEN("Destructive range turns on") {
                REQUIRE(state.getDestructiveRange() == true);
            }
        }
    }
}

// ============================================================================
// MIDI-mapped octave slider: a rest step must stay a rest
// ============================================================================
//
// handleOctaveSlider read the selected step's note and did note % 12 to keep
// the pitch class. On a rest (-1), -1 % 12 == -1 in C++, so the "pitch
// class" was -1 and the new note became octave*12 - 1 — e.g. CC value 127
// turned a rest into B7 (note 107). The UI hides the octave slider on rest
// steps; the MIDI path must be a no-op for the same reason.

SCENARIO("MIDI-mapped octave slider ignores rest steps",
         "[regression][midi-routing][rest]") {
    GamblerTestInitialiser juceInit;

    GIVEN("A step that is a rest, with a CC mapped to the octave slider") {
        TestApp app;
        auto& state = app.state();

        state.setSelectedStepForEditing(0);
        state.setStepNote(0, 0, -1);
        REQUIRE(state.getStepNote(0, 0) == -1);

        app.mapMidiCC(MidiControlId::OCTAVE_SLIDER, 20);

        WHEN("The mapped CC arrives at full value") {
            app.sendMidiCC(20, 127);

            THEN("The step is still a rest (not converted to a garbage note)") {
                REQUIRE(state.getStepNote(0, 0) == -1);
            }
        }

        WHEN("The mapped CC arrives at zero") {
            app.sendMidiCC(20, 0);

            THEN("The step is still a rest") {
                REQUIRE(state.getStepNote(0, 0) == -1);
            }
        }
    }
}
