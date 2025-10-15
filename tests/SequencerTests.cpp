#include <catch2/catch_test_macros.hpp>
#include <sequencer/Sequencer.h>
#include "helpers/test_helpers.h"

TEST_CASE("getCurrentStep() retrieves the raw parameter value of ", "[sequencer]")
{
    PluginProcessor plugin;
    auto apvts = createTestApvts(plugin);
    Sequencer seq(apvts);
    Step step = seq.getCurrentStep();
    REQUIRE(step.midiNote == 60);
}

