#include "backend/Sequence.h"
#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sequencer/Sequencer.h>
#include <vector>

TEST_CASE ("Gambler") {
    SECTION ("Sequence") {
        Sequence sequence;
        SECTION ("Has steps") {
        }
    }
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
        SECTION ("tick") {

        }
    }

    SECTION ("Plugin instance", "[instance]") {
        PluginProcessor plugin;

        SECTION ("name") {
            CHECK_THAT (plugin.getName().toStdString(),
                Catch::Matchers::Equals ("Gambler"));
        }
    }
}
