#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sequencer/SequencerService.h>

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



    SECTION ("Plugin instance", "[instance]")
    {
        SECTION ("name")
        {
            CHECK_THAT (plugin.getName().toStdString(),
                Catch::Matchers::Equals ("Gambler"));
        }
    }
}
