#include "backend/Sequence.h"
#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sequencer/Sequencer.h>
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
                REQUIRE (nextNote.notes[0] == 13);
            }
            SECTION ("tick() 128 times always gives you the next expected note") {
                for (int i = 1; i < 128; i++) {  // Should be 128, not 127
                    auto tickedNote = sequence.tick();
                    REQUIRE (tickedNote.notes[0] == i); // Check it returns 0, 1, 2... 127
                }
            }

            SECTION ("The sequence is restarted at correct boundary") {
                // Tick through the entire sequence (128 steps)
                for (int i = 1; i < 128; i++) {
                    sequence.tick();
                }

                // At this point, index should be at 127
                REQUIRE (sequence.index == 127);

                // One more tick should wrap around to 0
                auto wrappedNote = sequence.tick();
                REQUIRE (sequence.index == 0);
                REQUIRE (wrappedNote.notes[0] == 0);

                // Verify it continues from 0
                auto nextNote = sequence.tick();
                REQUIRE (sequence.index == 1);
                REQUIRE (nextNote.notes[0] == 1);
            }

            SECTION ("The sequence wraps at custom length boundary") {
                // Set a custom length (e.g., 16 steps)
                sequence.length = 16;

                // Tick through 15 steps (starting from 0, we'll be at index 15)
                for (int i = 1; i < 16; i++) {
                    sequence.tick();
                }

                // At this point, index should be at 15
                REQUIRE (sequence.index == 15);

                // One more tick should wrap around to 0
                auto wrappedNote = sequence.tick();
                REQUIRE (sequence.index == 0);
                REQUIRE (wrappedNote.notes[0] == 0);

                // Verify it continues from 0
                auto nextNote = sequence.tick();
                REQUIRE (sequence.index == 1);
                REQUIRE (nextNote.notes[0] == 1);
            }
       }
    }
}
