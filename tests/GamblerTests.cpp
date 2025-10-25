#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sequencer/Sequencer.h>
#include <vector>

TEST_CASE ("Step") {
    SECTION("Given a single lone step") {
        Step step;
        SECTION ("When the subStepLength is 1") {
            // Step step
            // step.setLength(1)
            // var noteFirst = seq.getNote()
            // seq.advance()
            // var noteNext = seq.getNote()
            // REQUIRE(noteNext == noteFirst)


        }
    }


}
TEST_CASE ("Sequencer") {
    PluginProcessor plugin;

    SECTION ("basics") {
        SECTION ("Step") {
            // TODO: test that we can getMidiNote()
            // and it will even go                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             grab dddddddddddddddddddq
        }
    }

    SECTION ("Given a sequencer with default settings") {
        Sequencer sequencer;

        SECTION ("When advance is called") {
            SECTION ("Then the step index increments by 1") {
                int initialStepIndex = sequencer.getStepIndex();
                sequencer.advance();
                REQUIRE (sequencer.getStepIndex() == (initialStepIndex + 1));
            }
        }

        SECTION ("When advance is called <stepLength> times") {
            SECTION ("Then the step index wraps back to 0") {
                int initialStepIndex = sequencer.getStepIndex();
                int stepLength = sequencer.getStepLength();

                for (int i = 0; i < stepLength; ++i) {
                    REQUIRE (sequencer.getStepIndex() == (initialStepIndex + i));
                    sequencer.advance();
                }

                REQUIRE (sequencer.getStepIndex() == 0);
            }
        }
    }

    SECTION ("Given a sequencer") {
        Sequencer sequencer;

        SECTION ("When random mode is enabled") {
            sequencer.getControls().setRandom (true);

            SECTION ("Then notes change randomly on each advance") {
                // Store initial notes for all steps
                std::vector<int> initialNotes;
                for (int i = 0; i < sequencer.getStepLength(); ++i) {
                    initialNotes.push_back (sequencer.getStep (i).getMidiNote());
                }

                // Advance through all steps once
                for (int i = 0; i < sequencer.getStepLength(); ++i) {
                    sequencer.advance();
                }

                // Count how many notes changed
                int changedCount = 0;
                for (int i = 0; i < sequencer.getStepLength(); ++i) {
                    if (sequencer.getStep (i).getMidiNote() != initialNotes[i]) {
                        changedCount++;
                    }
                }

                // With random mode, we expect most notes to have changed
                // (statistically, at least 90% should be different)
                REQUIRE (changedCount > sequencer.getStepLength() * 0.9);
            }

            SECTION ("Then all random notes are within valid MIDI range") {
                // Advance through multiple cycles to test randomization
                for (int cycle = 0; cycle < 5; ++cycle) {
                    for (int i = 0; i < sequencer.getStepLength(); ++i) {
                        sequencer.advance();

                        // Check that the note is within valid MIDI range (0-127)
                        int note = sequencer.getCurrentStep().getMidiNote();
                        REQUIRE (note >= 0);
                        REQUIRE (note <= 127);
                    }
                }
            }
        }

        SECTION ("When random mode is disabled") {
            sequencer.getControls().setRandom (false);

            SECTION ("Then notes remain unchanged after advancing") {
                // Store initial notes
                std::vector<int> initialNotes;
                for (int i = 0; i < sequencer.getStepLength(); ++i) {
                    initialNotes.push_back (sequencer.getStep (i).getMidiNote());
                }

                // Advance through all steps once
                for (int i = 0; i < sequencer.getStepLength(); ++i) {
                    sequencer.advance();
                }

                // Check that all notes remain unchanged
                for (int i = 0; i < sequencer.getStepLength(); ++i) {
                    REQUIRE (sequencer.getStep (i).getMidiNote() == initialNotes[i]);
                }
            }
        }
    }

    // SECTION ("Plugin instance", "[instance]")
    // {
    //     SECTION ("name")
    //     {
    //         CHECK_THAT (plugin.getName().toStdString(),
    //             Catch::Matchers::Equals ("Gambler"));
    //     }
    // }
}
