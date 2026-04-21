#include "algorithms/probability/EdenGrowthPattern.h"
#include "algorithms/probability/Grid.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>

using algorithms::probability::EdenGrowthPattern;
using algorithms::probability::Grid;

namespace {

constexpr int defaultDim = Grid::maxDim;

int countActive(const EdenGrowthPattern::Cells& cells, int upTo) {
    int n = 0;
    for (int i = 0; i < upTo; ++i)
        if (cells[static_cast<std::size_t>(i)]) ++n;
    return n;
}

} // namespace

TEST_CASE("EdenGrowthPattern starts empty", "[algorithms][eden]") {
    EdenGrowthPattern pattern(defaultDim, 42);
    REQUIRE(pattern.activeCount() == 0);
    REQUIRE(countActive(pattern.cells(), pattern.gridCellCount()) == 0);
}

TEST_CASE("EdenGrowthPattern reaches exact requested count", "[algorithms][eden]") {
    EdenGrowthPattern pattern(defaultDim, 42);

    for (int target : {1, 7, 32, 63, 64}) {
        pattern.setActiveCount(target);
        REQUIRE(pattern.activeCount() == target);
        REQUIRE(countActive(pattern.cells(), pattern.gridCellCount()) == target);
    }
}

TEST_CASE("EdenGrowthPattern clamps out-of-range counts", "[algorithms][eden]") {
    EdenGrowthPattern pattern(defaultDim, 1);

    pattern.setActiveCount(-5);
    REQUIRE(pattern.activeCount() == 0);

    pattern.setActiveCount(9999);
    REQUIRE(pattern.activeCount() == pattern.gridCellCount());
}

TEST_CASE("EdenGrowthPattern grows monotonically", "[algorithms][eden]") {
    EdenGrowthPattern pattern(defaultDim, 7);

    pattern.setActiveCount(10);
    const auto snapshot = pattern.cells();

    pattern.setActiveCount(20);
    const auto& grown = pattern.cells();

    for (std::size_t i = 0; i < snapshot.size(); ++i)
        if (snapshot[i]) REQUIRE(grown[i]);

    REQUIRE(countActive(grown, pattern.gridCellCount()) == 20);
}

TEST_CASE("EdenGrowthPattern shrink reverses growth", "[algorithms][eden]") {
    EdenGrowthPattern pattern(defaultDim, 99);

    pattern.setActiveCount(15);
    const auto target = pattern.cells();

    pattern.setActiveCount(40);
    pattern.setActiveCount(15);

    REQUIRE(pattern.cells() == target);
}

TEST_CASE("EdenGrowthPattern reseed clears state and resets RNG", "[algorithms][eden]") {
    EdenGrowthPattern a(defaultDim, 2024);
    a.setActiveCount(20);

    EdenGrowthPattern b(defaultDim, 2024);
    b.setActiveCount(20);

    REQUIRE(a.cells() == b.cells());

    a.reseed(2024);
    REQUIRE(a.activeCount() == 0);
    a.setActiveCount(20);
    REQUIRE(a.cells() == b.cells());
}

TEST_CASE("EdenGrowthPattern setActiveCount(0) clears", "[algorithms][eden]") {
    EdenGrowthPattern pattern(defaultDim, 5);
    pattern.setActiveCount(30);
    pattern.setActiveCount(0);
    REQUIRE(pattern.activeCount() == 0);
    REQUIRE(countActive(pattern.cells(), pattern.gridCellCount()) == 0);
}

TEST_CASE("EdenGrowthPattern works on smaller grids", "[algorithms][eden]") {
    SECTION("4x4 grid clamps to 16") {
        EdenGrowthPattern pattern(4, 11);
        pattern.setActiveCount(8);
        REQUIRE(pattern.activeCount() == 8);
        REQUIRE(countActive(pattern.cells(), 16) == 8);

        pattern.setActiveCount(999);
        REQUIRE(pattern.activeCount() == 16);
    }

    SECTION("2x2 grid only ever holds 4 cells") {
        EdenGrowthPattern pattern(2, 22);
        pattern.setActiveCount(3);
        REQUIRE(pattern.activeCount() == 3);
        REQUIRE(countActive(pattern.cells(), 4) == 3);
    }
}

TEST_CASE("EdenGrowthPattern setGridDim resets cluster", "[algorithms][eden]") {
    EdenGrowthPattern pattern(defaultDim, 42);
    pattern.setActiveCount(20);
    REQUIRE(pattern.activeCount() == 20);

    pattern.setGridDim(4);
    REQUIRE(pattern.gridDim() == 4);
    REQUIRE(pattern.activeCount() == 0);

    pattern.setActiveCount(8);
    REQUIRE(pattern.activeCount() == 8);
}
