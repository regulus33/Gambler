#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <sequencer/SequencerService.h>

#define APVTS_INTEGRATION "APVTS Integration";

TEST_CASE ("Gambler")
{
    PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    SequencerRepository repo (apvts);
    SequencerService sequencerService (repo);
    SECTION ("SequencerService sequencer movement")
    {
        SECTION ("Advancing the sequencer advances the step index")
        {
            int initialStepIndex = repo.getStepIndex();
            sequencerService.advance();
            REQUIRE (repo.getStepIndex() == (initialStepIndex + 1));
        }

        SECTION ("Advancing the sequencer enough times will wrap the index back to 0")
        {
            int initialStepIndex = repo.getStepIndex();
            int stepLength = repo.getStepLength();
            for (int i = 0; i < stepLength; ++i)
            {
                REQUIRE (repo.getStepIndex() == (initialStepIndex + i));
                sequencerService.advance();
            }

            REQUIRE (repo.getStepIndex() == 0);
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
