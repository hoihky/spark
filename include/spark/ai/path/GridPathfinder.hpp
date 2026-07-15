#pragma once

#include "spark/ai/path/IGridWalkability.hpp"
#include "spark/core/Array.hpp"
#include "spark/math/Vector2.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/** Uniform grid backed by a dense Array<uint8_t> (1 = blocked, 0 = free). */
class GridBitmapWalkability final : public IGridWalkability {
public:
    void Resize(const std::int32_t w, const std::int32_t h);
    void SetBlocked(const std::int32_t x, const std::int32_t y, const bool blocked) noexcept;

    [[nodiscard]] bool IsWalkable(const std::int32_t x, const std::int32_t y) const noexcept override;
    [[nodiscard]] std::int32_t Width() const noexcept override { return width; }
    [[nodiscard]] std::int32_t Height() const noexcept override { return height; }

private:
    [[nodiscard]] std::size_t Index_(const std::int32_t x, const std::int32_t y) const noexcept;

    std::int32_t width = 0;
    std::int32_t height = 0;
    Array<std::uint8_t> cells;
};

/**
 * 4-connected A* on an IGridWalkability. Output is grid cell centers in the same coordinate
 * convention: cell (x,y) maps to world XZ via caller-supplied origin and cellSize.
 */
class GridPathfinder {
public:
    struct Cell {
        std::int32_t x = 0;
        std::int32_t y = 0;
    };

    /** Returns false if no path; otherwise fills outCells from start to goal inclusive. */
    [[nodiscard]] static bool FindPath4(
            const IGridWalkability& grid,
            const Cell& start,
            const Cell& goal,
            Array<Cell>& outCells);

    /** Converts grid path to world XZ polyline (x = world X, y = world Z). */
    static void CellsToWorldPolyline(
            const Array<Cell>& cells,
            const Vector2& gridOriginXZ,
            const float cellSize,
            Array<Vector2>& outWorldXZ);
};

}  // namespace Spark
