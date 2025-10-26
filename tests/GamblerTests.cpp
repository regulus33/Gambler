#include "backend/Sequence.h"
#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <vector>



// Seed once at program start

TEST_CASE ("Gambler") {
    // over engineering - using some random numbers later so we need to seed the srand lib

    SECTION ("Note") {
        Note note;
        SECTION ("Has a vector of ints representing notes") {
            bool isDefaultLength = note.notes.size() == DEFAULT_MIDI_NOTE_LENGTH;
            REQUIRE (isDefaultLength);
        }
        SECTION ("Has a midi note length that can be changed") {
            SECTION ("Has default length") {
                REQUIRE (note.length == DEFAULT_MIDI_NOTE_LENGTH);
            }

            SECTION ("Can be increased") {
                auto initLength = note.length;
                note.increaseMidiNotes();
                auto newLength = note.length;
                REQUIRE (newLength == initLength + 1);

                SECTION ("but not more than the max subsequence length") {
                    for (int i = 0; i < MAX_MIDI_NOTE_LENGTH; i++) {
                        note.increaseMidiNotes();
                    }

                    auto finalLength = note.length;
                    REQUIRE (finalLength == MAX_MIDI_NOTE_LENGTH);
                }
            }

            SECTION ("Can be decreased") {
                auto initLength = note.length;
                note.decreaseMidiNotes();
                auto newLength = note.length;
                REQUIRE (newLength == initLength);

                SECTION ("but not less than the min subsequence length") {
                    for (int i = 0; i < MAX_MIDI_NOTE_LENGTH; i++) {
                        note.decreaseMidiNotes();
                    }

                    auto finalLength = note.length;
                    REQUIRE (finalLength == 1);
                }
            }
        }

        SECTION ("setNote()") {
            SECTION ("Allows you to set a note at any index,") {
                auto initialZeroNote = note.notes[0];
                REQUIRE (initialZeroNote == DEFAULT_MIDI_NOTE);
                note.setNote (0, 127);
                REQUIRE (note.notes[0] == 127);
                SECTION ("but never lets you set an invalid midi note") {
                    note.setNote (0, 999);
                    REQUIRE (note.notes[0] == 127);
                }
                SECTION ("or a note index that is outside the length range") {
                    int bigIndex = 11;
                    int noteInt = note.notes[bigIndex];
                    note.setNote (bigIndex, 30);
                    REQUIRE (note.notes[bigIndex] != 30);
                }
            }
        }

        SECTION ("tick()") {
            SECTION ("tick returns the next note") {
                int noteNum = note.tick();
                REQUIRE (noteNum == DEFAULT_MIDI_NOTE);
            }
        }

        SECTION ("Slots") {
            SECTION ("Has 7 slots") {
                REQUIRE (note.slots.size() == NUM_SLOTS);
            }

            SECTION ("First slot defaults to Velocity type") {
                REQUIRE (note.slots[0].type == SlotType::Velocity);
                REQUIRE (note.slots[0].value == DEFAULT_VELOCITY);
            }

            SECTION ("Other slots default to CC type") {
                for (int i = 1; i < NUM_SLOTS; i++) {
                    REQUIRE (note.slots[i].type == SlotType::CC);
                    REQUIRE (note.slots[i].ccNumber == i);
                    REQUIRE (note.slots[i].value == 64);
                }
            }

            SECTION ("Can set slot type") {
                note.setSlotType(0, SlotType::CC);
                REQUIRE (note.slots[0].type == SlotType::CC);

                note.setSlotType(1, SlotType::Velocity);
                REQUIRE (note.slots[1].type == SlotType::Velocity);
            }

            SECTION ("Can set slot CC number") {
                note.setSlotCCNumber(0, 74); // Filter cutoff
                REQUIRE (note.slots[0].ccNumber == 74);

                SECTION ("but not invalid CC numbers") {
                    note.setSlotCCNumber(0, 200);
                    REQUIRE (note.slots[0].ccNumber == 74); // Should remain unchanged
                }
            }

            SECTION ("Can set slot value") {
                note.setSlotValue(0, 127);
                REQUIRE (note.slots[0].value == 127);

                note.setSlotValue(1, 0);
                REQUIRE (note.slots[1].value == 0);

                SECTION ("but not invalid values") {
                    note.setSlotValue(0, 200);
                    REQUIRE (note.slots[0].value == 127); // Should remain unchanged
                }
            }

            SECTION ("Can configure a slot for different CC types") {
                // Configure slot 0 for filter cutoff (CC 74)
                note.setSlotType(0, SlotType::CC);
                note.setSlotCCNumber(0, 74);
                note.setSlotValue(0, 100);

                REQUIRE (note.slots[0].type == SlotType::CC);
                REQUIRE (note.slots[0].ccNumber == 74);
                REQUIRE (note.slots[0].value == 100);

                // Configure slot 1 for velocity
                note.setSlotType(1, SlotType::Velocity);
                note.setSlotValue(1, 127);

                REQUIRE (note.slots[1].type == SlotType::Velocity);
                REQUIRE (note.slots[1].value == 127);
            }
        }
    }

    SECTION ("Sequence") {
        Sequence sequence;
        SECTION ("Setting a note in Sequence just sets Note at index's first note in Note's note vector") {
            sequence.setNote (0, 33);
            REQUIRE (sequence.notes[0].notes[0] == 33);
        }
        SECTION ("Sequencer Notes") {
            // Setup all the notes 0 - 127 with midi notes 0 - 127
            for (int i = 0; i < 128; i++) {
                sequence.setNote (i, i);
            }

            // check that some random note is what we'd expect based on index.
            auto someIndex = getRandFromZeroToExcluding(128);
            auto expectedNote = someIndex;
            auto note = sequence.notes[someIndex];

            REQUIRE (note.notes[0] == expectedNote);

            SECTION ("tick() returns note in NEXT index") {
                sequence.index = 12;
                REQUIRE (sequence.notes[12].notes[0] == 12);
                auto nextNote = sequence.tick();
                REQUIRE (nextNote == 13);
            }
            SECTION ("tick() 128 times always gives you the next expected note") {
                for (int i = 0; i < 128; i++) {
                    auto tickedNote = sequence.tick();
                    REQUIRE (tickedNote == i); // Check it returns 0, 1, 2, 3... 127
                }
            }

            SECTION ("The sequence is restarted at correct boundary") {
                // Tick through the entire sequence (128 steps)
                for (int i = 0; i < 127; i++) {
                    sequence.tick();
                }

                // At this point, index should be at 126 (we've ticked 127 times from -1)
                REQUIRE (sequence.index == 126);

                // One more tick should wrap around to 127
                auto note127 = sequence.tick();
                REQUIRE (sequence.index == 127);
                REQUIRE (note127 == 127);

                // One more tick should wrap around to 0
                auto wrappedNote = sequence.tick();
                REQUIRE (sequence.index == 0);
                REQUIRE (wrappedNote == 0);

                // Verify it continues from 0
                auto nextNote = sequence.tick();
                REQUIRE (sequence.index == 1);
                REQUIRE (nextNote == 1);
            }

            SECTION ("The sequence wraps at custom length boundary") {
                // Set a custom length (e.g., 16 steps)
                sequence.length = 16;

                // Tick through 15 steps (starting from -1, we'll be at index 14)
                for (int i = 0; i < 15; i++) {
                    sequence.tick();
                }

                // At this point, index should be at 14 (we've ticked 15 times from -1)
                REQUIRE (sequence.index == 14);

                // One more tick should get us to 15
                auto note15 = sequence.tick();
                REQUIRE (sequence.index == 15);
                REQUIRE (note15 == 15);

                // One more tick should wrap around to 0
                auto wrappedNote = sequence.tick();
                REQUIRE (sequence.index == 0);
                REQUIRE (wrappedNote == 0);

                // Verify it continues from 0
                auto nextNote = sequence.tick();
                REQUIRE (sequence.index == 1);
                REQUIRE (nextNote == 1);
            }

            SECTION ("Notes with subsequences create evolving patterns") {
                // Create a 4-step sequence
                sequence.length = 4;

                // Steps 0-2: single note (60)
                sequence.setNote(0, 60);
                sequence.setNote(1, 60);
                sequence.setNote(2, 60);

                // Step 3: 2-note subsequence [60, 70]
                sequence.notes[3].increaseMidiNotes();  // Increase length from 1 to 2
                sequence.notes[3].setNote(0, 60);
                sequence.notes[3].setNote(1, 70);

                // First pass through the sequence
                REQUIRE (sequence.tick() == 60); // step 0
                REQUIRE (sequence.tick() == 60); // step 1
                REQUIRE (sequence.tick() == 60); // step 2
                REQUIRE (sequence.tick() == 60); // step 3, substep 0

                // Second pass - step 3 should now play substep 1
                REQUIRE (sequence.tick() == 60); // step 0
                REQUIRE (sequence.tick() == 60); // step 1
                REQUIRE (sequence.tick() == 60); // step 2
                REQUIRE (sequence.tick() == 70); // step 3, substep 1

                // Third pass - step 3 wraps back to substep 0
                REQUIRE (sequence.tick() == 60); // step 0
                REQUIRE (sequence.tick() == 60); // step 1
                REQUIRE (sequence.tick() == 60); // step 2
                REQUIRE (sequence.tick() == 60); // step 3, substep 0 again
            }

            SECTION ("Sequence index never returns to -1 after first tick") {
                // Initial state should be -1
                REQUIRE (sequence.index == -1);

                // After first tick, should be 0
                sequence.tick();
                REQUIRE (sequence.index == 0);

                // Tick through entire sequence multiple times
                for (int pass = 0; pass < 3; pass++) {
                    for (int i = 0; i < 128; i++) {
                        sequence.tick();
                        // Index should never be -1 after initialization
                        REQUIRE (sequence.index >= 0);
                        REQUIRE (sequence.index < 128);
                    }
                }

                // After wrapping, should be at 0, not -1
                REQUIRE (sequence.index == 0);
            }
       }

        SECTION ("Note") {
            SECTION ("Note index never returns to -1 after first tick") {
                Note note;
                note.increaseMidiNotes();  // Length = 2
                note.setNote(0, 60);
                note.setNote(1, 70);

                // Initial state should be -1
                REQUIRE (note.index == -1);

                // After first tick, should be 0
                note.tick();
                REQUIRE (note.index == 0);

                // Tick through multiple cycles
                for (int cycle = 0; cycle < 10; cycle++) {
                    note.tick();
                    // Index should never be -1 after initialization
                    REQUIRE (note.index >= 0);
                    REQUIRE (note.index < note.length);
                }

                // After wrapping, should be at 0, not -1
                REQUIRE (note.index == 0);
            }
        }
    }
}
