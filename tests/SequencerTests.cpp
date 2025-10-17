#include <catch2/catch_test_macros.hpp>
#include <sequencer/Sequencer.h>
#include "helpers/test_helpers.h"


#define APVTS_INTEGRATION "APVTS Integration";

TEST_CASE("APVTS Integration", "[apvts]")
{
    SECTION ("Makes the thing work some way")
    {
        PluginProcessor plugin;
        auto& apvts = plugin.getAPVTS();
        Sequencer seq(apvts);
        Step step = seq.getCurrentStep();
        REQUIRE(step.midiNote == 99);
        SECTION ("Makes the MIDI note work some way")
        {
            DBG("ran");
            REQUIRE(step.midiNote == 99);
        }
    }
}
