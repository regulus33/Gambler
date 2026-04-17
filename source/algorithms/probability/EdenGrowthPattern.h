#pragma once

#include "Grid8x8.h"

#include <array>
#include <cstdint>
#include <random>
#include <set>
#include <vector>

namespace algorithms::probability {

// Stateful Eden growth model on Grid8x8.
//
// Eden growth: plant a random seed cell, then repeatedly activate a random
// inactive cell that is adjacent to the existing cluster. Produces organic
// blob-like shapes that look "grown" rather than scattered.
//
// The model is monotonic: increasing the active count adds cells to the
// existing cluster; decreasing pops cells in reverse growth order, so the
// shape shrinks back along the path it grew.
class EdenGrowthPattern {
public:
    using Cells = std::array<bool, Grid8x8::cellCount>;

    explicit EdenGrowthPattern (std::uint32_t seed = std::random_device{}()) {
        rng_.seed (seed);
        cells_.fill (false);
    }

    // Re-seed the RNG and clear the cluster.
    void reseed (std::uint32_t seed) {
        rng_.seed (seed);
        clear();
    }

    // Forget the current cluster. Next grow() will pick a fresh seed cell.
    void clear() {
        cells_.fill (false);
        growthOrder_.clear();
        boundary_.clear();
    }

    // Resize the cluster to exactly `count` active cells (clamped to [0, 64]).
    // Grows or shrinks as needed; preserves existing cells when possible.
    void setActiveCount (int count) {
        const int target = clamp (count);

        if (target == 0) {
            clear();
            return;
        }

        while (activeCount() > target) shrinkOne();
        while (activeCount() < target) growOne();
    }

    int activeCount() const noexcept {
        return static_cast<int> (growthOrder_.size());
    }

    const Cells& cells() const noexcept { return cells_; }

private:
    static int clamp (int n) noexcept {
        if (n < 0) return 0;
        if (n > Grid8x8::cellCount) return Grid8x8::cellCount;
        return n;
    }

    void growOne() {
        const int next = growthOrder_.empty()
                             ? pickSeed()
                             : pickFromBoundary();

        cells_[static_cast<std::size_t> (next)] = true;
        growthOrder_.push_back (next);
        boundary_.erase (next);

        std::array<int, 4> nbrs{};
        const auto count = Grid8x8::neighbours (next, nbrs);
        for (std::size_t i = 0; i < count; ++i) {
            const int idx = nbrs[i];
            if (! cells_[static_cast<std::size_t> (idx)])
                boundary_.insert (idx);
        }
    }

    void shrinkOne() {
        const int last = growthOrder_.back();
        growthOrder_.pop_back();
        cells_[static_cast<std::size_t> (last)] = false;

        // The removed cell rejoins the boundary if it still touches the cluster.
        if (hasActiveNeighbour (last))
            boundary_.insert (last);

        // Each neighbour that was only in the boundary because of the removed
        // cell must be re-evaluated.
        std::array<int, 4> nbrs{};
        const auto count = Grid8x8::neighbours (last, nbrs);
        for (std::size_t i = 0; i < count; ++i) {
            const int idx = nbrs[i];
            if (cells_[static_cast<std::size_t> (idx)]) continue;
            if (! hasActiveNeighbour (idx))
                boundary_.erase (idx);
        }
    }

    int pickSeed() {
        std::uniform_int_distribution<int> dist (0, Grid8x8::cellCount - 1);
        return dist (rng_);
    }

    int pickFromBoundary() {
        // Defensive: if the cluster has somehow eaten its own boundary
        // (shouldn't happen on a connected grid below full capacity), fall
        // back to picking any inactive cell at random.
        if (boundary_.empty())
            return pickAnyInactive();

        std::uniform_int_distribution<std::size_t> dist (0, boundary_.size() - 1);
        auto it = boundary_.begin();
        std::advance (it, dist (rng_));
        return *it;
    }

    int pickAnyInactive() {
        std::vector<int> inactive;
        inactive.reserve (Grid8x8::cellCount);
        for (int i = 0; i < Grid8x8::cellCount; ++i)
            if (! cells_[static_cast<std::size_t> (i)])
                inactive.push_back (i);

        std::uniform_int_distribution<std::size_t> dist (0, inactive.size() - 1);
        return inactive[dist (rng_)];
    }

    bool hasActiveNeighbour (int index) const {
        std::array<int, 4> nbrs{};
        const auto count = Grid8x8::neighbours (index, nbrs);
        for (std::size_t i = 0; i < count; ++i)
            if (cells_[static_cast<std::size_t> (nbrs[i])])
                return true;
        return false;
    }

    Cells cells_{};
    std::vector<int> growthOrder_;
    std::set<int> boundary_;
    std::mt19937 rng_;
};

} // namespace algorithms::probability
