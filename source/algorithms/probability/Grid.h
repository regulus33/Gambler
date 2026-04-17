#pragma once

#include <array>
#include <cassert>
#include <cstddef>

namespace algorithms::probability {

// A square bounded grid (no wrap) with runtime dimension.
// Dim must be >= 1; for this project the valid dims are 2, 4, 8.
// Cell index = row * dim + col, row 0 at top.
class Grid {
public:
    static constexpr int maxDim = 8;
    static constexpr int maxCellCount = maxDim * maxDim;

    struct Coord { int row; int col; };

    explicit Grid (int dim) noexcept : dim_ (dim) {
        assert (dim >= 1 && dim <= maxDim);
    }

    int dim() const noexcept { return dim_; }
    int cellCount() const noexcept { return dim_ * dim_; }

    bool inBounds (int row, int col) const noexcept {
        return row >= 0 && row < dim_ && col >= 0 && col < dim_;
    }

    int toIndex (int row, int col) const noexcept {
        return row * dim_ + col;
    }

    Coord toCoord (int index) const noexcept {
        return { index / dim_, index % dim_ };
    }

    // Writes von-Neumann (4-direction) neighbour indices into `out`.
    // Returns the number written (2 at corners, 3 at edges, 4 in the interior).
    std::size_t neighbours (int index, std::array<int, 4>& out) const noexcept {
        const auto [row, col] = toCoord (index);
        std::size_t n = 0;
        if (inBounds (row - 1, col)) out[n++] = toIndex (row - 1, col);
        if (inBounds (row + 1, col)) out[n++] = toIndex (row + 1, col);
        if (inBounds (row, col - 1)) out[n++] = toIndex (row, col - 1);
        if (inBounds (row, col + 1)) out[n++] = toIndex (row, col + 1);
        return n;
    }

private:
    int dim_;
};

} // namespace algorithms::probability
