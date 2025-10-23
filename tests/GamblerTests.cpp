#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sequencer/Sequencer.h>
#include <vector>

TEST_CASE ("Gambler")
{
    PluginProcessor plugin;

    SECTION ("Sequencer movement")
    {
        Sequencer sequencer;

        SECTION ("Advancing the sequencer advances the step index")
        {
            int initialStepIndex = sequencer.getStepIndex();
            sequencer.advance();
            REQUIRE (sequencer.getStepIndex() == (initialStepIndex + 1));
        }

        SECTION ("Advancing the sequencer enough times will wrap the index back to 0")
        {
            int initialStepIndex = sequencer.getStepIndex();
            int stepLength = sequencer.getStepLength();
            for (int i = 0; i < stepLength; ++i)
            {
                REQUIRE (sequencer.getStepIndex() == (initialStepIndex + i));
                sequencer.advance();
            }

            REQUIRE (sequencer.getStepIndex() == 0);
        }
    }

    SECTION ("Sequencer random note changes")
    {
        Sequencer sequencer;

        SECTION ("Notes change randomly when random mode is enabled")
        {
            // Store initial notes for the first few steps
            std::vector<int> initialNotes;
            for (int i = 0; i < sequencer.getStepLength(); ++i)
            {
                initialNotes.push_back(sequencer.getStep(i).midiNote);
            }

            // Enable random mode
            sequencer.getControls().setRandom(true);

            // Advance through all steps once
            for (int i = 0; i < sequencer.getStepLength(); ++i)
            {
                sequencer.advance();
            }

            // Check that at least some notes have changed
            int changedCount = 0;
            for (int i = 0; i < sequencer.getStepLength(); ++i)
            {
                if (sequencer.getStep(i).midiNote != initialNotes[i])
                {
                    changedCount++;
                }
            }

            // With random mode, we expect most notes to have changed
            // (statistically, at least 90% should be different)
            REQUIRE (changedCount > sequencer.getStepLength() * 0.9);
        }

        SECTION ("Notes remain unchanged when random mode is disabled")
        {
            // Ensure random mode is disabled (default state)
            sequencer.getControls().setRandom(false);

            // Store initial notes
            std::vector<int> initialNotes;
            for (int i = 0; i < sequencer.getStepLength(); ++i)
            {
                initialNotes.push_back(sequencer.getStep(i).midiNote);
            }

            // Advance through all steps once
            for (int i = 0; i < sequencer.getStepLength(); ++i)
            {
                sequencer.advance();
            }

            // Check that all notes remain unchanged
            for (int i = 0; i < sequencer.getStepLength(); ++i)
            {
                REQUIRE (sequencer.getStep(i).midiNote == initialNotes[i]);
            }
        }

        SECTION ("Random notes are within valid MIDI range")
        {
            sequencer.getControls().setRandom(true);

            // Advance through multiple cycles to test randomization
            for (int cycle = 0; cycle < 5; ++cycle)
            {
                for (int i = 0; i < sequencer.getStepLength(); ++i)
                {
                    sequencer.advance();

                    // Check that the note is within valid MIDI range (0-127)
                    int note = sequencer.getCurrentStep().midiNote;
                    REQUIRE (note >= 0);
                    REQUIRE (note <= 127);
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
