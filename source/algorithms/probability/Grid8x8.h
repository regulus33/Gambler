#pragma once

#include <array>
#include <cstddef>

namespace algorithms::probability {

// Geometry helper for an 8x8 bounded grid (no wrap).
// Cell index = row * width + col, row 0 at top.
class Grid8x8 {
public:
    static constexpr int width = 8;
    static constexpr int height = 8;
    static constexpr int cellCount = width * height;

    struct Coord { int row; int col; };

    static constexpr bool inBounds (int row, int col) noexcept {
        return row >= 0 && row < height && col >= 0 && col < width;
    }

    static constexpr int toIndex (int row, int col) noexcept {
        return row * width + col;
    }

    static constexpr Coord toCoord (int index) noexcept {
        return { index / width, index % width };
    }

    // Writes von-Neumann (4-direction) neighbour indices into `out`.
    // Returns the number written (2 at corners, 3 at edges, 4 in the interior).
    static std::size_t neighbours (int index, std::array<int, 4>& out) noexcept {
        const auto [row, col] = toCoord (index);
        std::size_t n = 0;
        if (inBounds (row - 1, col)) out[n++] = toIndex (row - 1, col);
        if (inBounds (row + 1, col)) out[n++] = toIndex (row + 1, col);
        if (inBounds (row, col - 1)) out[n++] = toIndex (row, col - 1);
        if (inBounds (row, col + 1)) out[n++] = toIndex (row, col + 1);
        return n;
    }
};

} // namespace algorithms::probability
