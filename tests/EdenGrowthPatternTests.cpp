#include "algorithms/probability/EdenGrowthPattern.h"
#include "algorithms/probability/Grid8x8.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <queue>
#include <unordered_set>

using algorithms::probability::EdenGrowthPattern;
using algorithms::probability::Grid8x8;

namespace {

int countActive(const EdenGrowthPattern::Cells& cells) {
    int n = 0;
    for (bool on : cells) if (on) ++n;
    return n;
}

// Verify the active cells form a single connected region (4-neighbour).
bool isConnected(const EdenGrowthPattern::Cells& cells) {
    int seed = -1;
    for (int i = 0; i < Grid8x8::cellCount; ++i) {
        if (cells[static_cast<std::size_t>(i)]) { seed = i; break; }
    }
    if (seed < 0) return true; // empty cluster is trivially connected

    std::unordered_set<int> visited;
    std::queue<int> q;
    q.push(seed);
    visited.insert(seed);

    while (! q.empty()) {
        const int idx = q.front(); q.pop();
        std::array<int, 4> nbrs{};
        const auto count = Grid8x8::neighbours(idx, nbrs);
        for (std::size_t i = 0; i < count; ++i) {
            const int n = nbrs[i];
            if (cells[static_cast<std::size_t>(n)] && ! visited.count(n)) {
                visited.insert(n);
                q.push(n);
            }
        }
    }

    return static_cast<int>(visited.size()) == countActive(cells);
}

} // namespace

TEST_CASE("EdenGrowthPattern starts empty", "[algorithms][eden]") {
    EdenGrowthPattern pattern(42);
    REQUIRE(pattern.activeCount() == 0);
    REQUIRE(countActive(pattern.cells()) == 0);
}

TEST_CASE("EdenGrowthPattern reaches exact requested count", "[algorithms][eden]") {
    EdenGrowthPattern pattern(42);

    for (int target : {1, 7, 32, 63, 64}) {
        pattern.setActiveCount(target);
        REQUIRE(pattern.activeCount() == target);
        REQUIRE(countActive(pattern.cells()) == target);
    }
}

TEST_CASE("EdenGrowthPattern clamps out-of-range counts", "[algorithms][eden]") {
    EdenGrowthPattern pattern(1);

    pattern.setActiveCount(-5);
    REQUIRE(pattern.activeCount() == 0);

    pattern.setActiveCount(9999);
    REQUIRE(pattern.activeCount() == Grid8x8::cellCount);
}

TEST_CASE("EdenGrowthPattern produces a connected cluster", "[algorithms][eden]") {
    EdenGrowthPattern pattern(123);

    // Try a range of sizes — all should be one connected blob.
    for (int target : {1, 5, 12, 25, 40, 55}) {
        pattern.clear();
        pattern.setActiveCount(target);
        INFO("target = " << target);
        REQUIRE(isConnected(pattern.cells()));
    }
}

TEST_CASE("EdenGrowthPattern grows monotonically", "[algorithms][eden]") {
    EdenGrowthPattern pattern(7);

    pattern.setActiveCount(10);
    const auto snapshot = pattern.cells();

    pattern.setActiveCount(20);
    const auto& grown = pattern.cells();

    // Every cell active at count=10 should still be active at count=20.
    for (std::size_t i = 0; i < snapshot.size(); ++i) {
        if (snapshot[i]) REQUIRE(grown[i]);
    }
    REQUIRE(countActive(grown) == 20);
}

TEST_CASE("EdenGrowthPattern shrink reverses growth", "[algorithms][eden]") {
    EdenGrowthPattern pattern(99);

    pattern.setActiveCount(15);
    const auto target = pattern.cells();

    pattern.setActiveCount(40);
    pattern.setActiveCount(15);

    REQUIRE(pattern.cells() == target);
}

TEST_CASE("EdenGrowthPattern reseed clears state and resets RNG", "[algorithms][eden]") {
    EdenGrowthPattern a(2024);
    a.setActiveCount(20);

    EdenGrowthPattern b(2024);
    b.setActiveCount(20);

    REQUIRE(a.cells() == b.cells());

    a.reseed(2024);
    REQUIRE(a.activeCount() == 0);
    a.setActiveCount(20);
    REQUIRE(a.cells() == b.cells());
}

TEST_CASE("EdenGrowthPattern setActiveCount(0) clears", "[algorithms][eden]") {
    EdenGrowthPattern pattern(5);
    pattern.setActiveCount(30);
    pattern.setActiveCount(0);
    REQUIRE(pattern.activeCount() == 0);
    REQUIRE(countActive(pattern.cells()) == 0);
}
