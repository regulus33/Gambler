#include <catch2/catch_test_macros.hpp>
#include <sequencer/Sequencer.h>

TEST_CASE("Sequencer::test returns true", "[sequencer]")
{
    Sequencer seq;
    CHECK(seq.test());
}

