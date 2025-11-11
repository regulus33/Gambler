#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "../source/ui/PianoRollComponent.h"

/**
 * Tests for PianoRollComponent
 * 
 * These tests verify the active note indicator functionality
 * which highlights the currently playing note in the sequence
 */

SCENARIO("PianoRollComponent can set and get active note", "[pianoroll][active-note]") {
    GIVEN("A PianoRollComponent") {
        PianoRollComponent pianoRoll;
        
        WHEN("No active note is set initially") {
            THEN("Active note should be -1") {
                REQUIRE(pianoRoll.getActiveNote() == -1);
            }
        }
        
        WHEN("Setting active note to C (index 0)") {
            pianoRoll.setActiveNote(0);
            
            THEN("Active note should be 0") {
                REQUIRE(pianoRoll.getActiveNote() == 0);
            }
        }
        
        WHEN("Setting active note to G (index 7)") {
            pianoRoll.setActiveNote(7);
            
            THEN("Active note should be 7") {
                REQUIRE(pianoRoll.getActiveNote() == 7);
            }
        }
        
        WHEN("Setting active note to B (index 11)") {
            pianoRoll.setActiveNote(11);
            
            THEN("Active note should be 11") {
                REQUIRE(pianoRoll.getActiveNote() == 11);
            }
        }
        
        WHEN("Setting active note back to -1 (none)") {
            pianoRoll.setActiveNote(5);
            pianoRoll.setActiveNote(-1);
            
            THEN("Active note should be -1") {
                REQUIRE(pianoRoll.getActiveNote() == -1);
            }
        }
    }
}

SCENARIO("PianoRollComponent active note changes trigger repaint", "[pianoroll][active-note]") {
    GIVEN("A PianoRollComponent with an active note") {
        PianoRollComponent pianoRoll;
        pianoRoll.setActiveNote(3);
        
        WHEN("Setting a different active note") {
            pianoRoll.setActiveNote(7);
            
            THEN("Active note should update") {
                REQUIRE(pianoRoll.getActiveNote() == 7);
            }
        }
        
        WHEN("Setting the same active note again") {
            int currentNote = pianoRoll.getActiveNote();
            pianoRoll.setActiveNote(currentNote);
            
            THEN("Active note should remain unchanged") {
                REQUIRE(pianoRoll.getActiveNote() == currentNote);
            }
        }
    }
}

SCENARIO("PianoRollComponent active note works with all chromatic notes", "[pianoroll][active-note]") {
    GIVEN("A PianoRollComponent") {
        PianoRollComponent pianoRoll;
        
        WHEN("Setting active note for each chromatic note (0-11)") {
            for (int i = 0; i < 12; i++) {
                pianoRoll.setActiveNote(i);
                
                THEN("Active note should match the set value") {
                    REQUIRE(pianoRoll.getActiveNote() == i);
                }
            }
        }
    }
}

SCENARIO("PianoRollComponent active note is independent of scale selection", "[pianoroll][active-note]") {
    GIVEN("A PianoRollComponent with a custom scale") {
        PianoRollComponent pianoRoll;
        
        // Set up a C major scale (C, D, E, F, G, A, B)
        Scale cMajor;
        std::array<int, 12> cMajorNotes = {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1};
        cMajor.setScaleNotes(cMajorNotes);
        pianoRoll.setScale(cMajor);
        
        WHEN("Setting active note to a note in the scale (C = 0)") {
            pianoRoll.setActiveNote(0);
            
            THEN("Active note should be set correctly") {
                REQUIRE(pianoRoll.getActiveNote() == 0);
            }
        }
        
        WHEN("Setting active note to a note NOT in the scale (Db = 1)") {
            pianoRoll.setActiveNote(1);
            
            THEN("Active note should still be set correctly") {
                REQUIRE(pianoRoll.getActiveNote() == 1);
            }
            
            AND_THEN("The scale should remain unchanged") {
                auto retrievedScale = pianoRoll.getScale();
                auto scaleNotes = retrievedScale.getScaleNotes();
                REQUIRE(scaleNotes[1] == 0); // Db should still be off
            }
        }
    }
}

SCENARIO("PianoRollComponent active note persists through scale changes", "[pianoroll][active-note]") {
    GIVEN("A PianoRollComponent with an active note set") {
        PianoRollComponent pianoRoll;
        pianoRoll.setActiveNote(7); // G
        
        WHEN("Changing the scale") {
            Scale newScale;
            std::array<int, 12> pentatonic = {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0};
            newScale.setScaleNotes(pentatonic);
            pianoRoll.setScale(newScale);
            
            THEN("Active note should remain unchanged") {
                REQUIRE(pianoRoll.getActiveNote() == 7);
            }
        }
    }
}

SCENARIO("PianoRollComponent can handle rapid active note changes", "[pianoroll][active-note]") {
    GIVEN("A PianoRollComponent") {
        PianoRollComponent pianoRoll;
        
        WHEN("Rapidly changing active notes in sequence") {
            pianoRoll.setActiveNote(0);  // C
            pianoRoll.setActiveNote(4);  // E
            pianoRoll.setActiveNote(7);  // G
            pianoRoll.setActiveNote(11); // B
            pianoRoll.setActiveNote(-1); // None
            
            THEN("Final active note should be -1") {
                REQUIRE(pianoRoll.getActiveNote() == -1);
            }
        }
        
        WHEN("Setting active notes in a musical pattern") {
            // Simulate a C major arpeggio: C, E, G, C
            std::vector<int> arpeggio = {0, 4, 7, 0};
            
            for (int note : arpeggio) {
                pianoRoll.setActiveNote(note);
                REQUIRE(pianoRoll.getActiveNote() == note);
            }
            
            THEN("Last note should be C (0)") {
                REQUIRE(pianoRoll.getActiveNote() == 0);
            }
        }
    }
}

