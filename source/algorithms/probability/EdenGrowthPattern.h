#pragma once

#include "Grid.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace algorithms::probability {

// Stateful random-scatter pattern on a square Grid of runtime dimension.
//
// Each grow step activates a uniformly random inactive cell anywhere on the
// grid, producing scattered placement rather than a connected cluster.
//
// The model is monotonic: increasing the active count adds new random cells
// while preserving existing ones; decreasing pops cells in reverse order of
// insertion, so nudging the slider back recovers the exact previous shape.
class EdenGrowthPattern {
public:
    using Cells = std::array<bool, Grid::maxCellCount>;

    explicit EdenGrowthPattern (int gridDim = Grid::maxDim,
                                std::uint32_t seed = std::random_device{}())
        : grid_ (gridDim)
    {
        rng_.seed (seed);
        cells_.fill (false);
    }

    // Re-seed the RNG and clear the cluster.
    void reseed (std::uint32_t seed) {
        rng_.seed (seed);
        clear();
    }

    // Change the underlying grid dimension. Clears the cluster (cells that
    // made sense on the old grid wouldn't on the new one).
    void setGridDim (int newDim) {
        if (newDim == grid_.dim()) return;
        grid_ = Grid (newDim);
        clear();
    }

    int gridDim() const noexcept { return grid_.dim(); }
    int gridCellCount() const noexcept { return grid_.cellCount(); }

    // Forget the current pattern.
    void clear() {
        cells_.fill (false);
        growthOrder_.clear();
    }

    // Resize the cluster to exactly `count` active cells (clamped to
    // [0, gridCellCount()]). Grows or shrinks as needed; preserves existing
    // cells when possible.
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
    int clamp (int n) const noexcept {
        if (n < 0) return 0;
        if (n > grid_.cellCount()) return grid_.cellCount();
        return n;
    }

    void growOne() {
        // Pick the k-th inactive cell for a uniformly random k.
        const int n = grid_.cellCount();
        const int inactiveCount = n - activeCount();
        std::uniform_int_distribution<int> dist (0, inactiveCount - 1);
        int k = dist (rng_);

        int next = 0;
        for (; next < n; ++next) {
            if (cells_[static_cast<std::size_t> (next)]) continue;
            if (k == 0) break;
            --k;
        }

        cells_[static_cast<std::size_t> (next)] = true;
        growthOrder_.push_back (next);
    }

    void shrinkOne() {
        const int last = growthOrder_.back();
        growthOrder_.pop_back();
        cells_[static_cast<std::size_t> (last)] = false;
    }

    Grid grid_;
    Cells cells_{};
    std::vector<int> growthOrder_;
    std::mt19937 rng_;
};

} // namespace algorithms::probability
