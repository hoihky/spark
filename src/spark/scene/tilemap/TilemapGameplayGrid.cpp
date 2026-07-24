#include "spark/scene/tilemap/TilemapGameplayGrid.hpp"

namespace Spark {

void TilemapGameplayGrid::Resize(const std::int32_t w, const std::int32_t h) {
    width = w;
    height = h;
    blocked.Clear();
    if (w > 0 && h > 0) {
        const std::size_t n = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        blocked.Resize(n);
        for (std::size_t i = 0; i < n; ++i) {
            blocked[i] = 1;
        }
    }
}

void TilemapGameplayGrid::ClearAllWalkable() noexcept {
    for (std::size_t i = 0; i < blocked.GetSize(); ++i) {
        blocked[i] = 0;
    }
}

std::size_t TilemapGameplayGrid::CellIndex(const std::int32_t x, const std::int32_t y) const noexcept {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

void TilemapGameplayGrid::SetBlocked(const std::int32_t x, const std::int32_t y, const bool isBlocked) noexcept {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return;
    }
    blocked[CellIndex(x, y)] = isBlocked ? 1U : 0U;
}

bool TilemapGameplayGrid::IsWalkable(const std::int32_t x, const std::int32_t y) const noexcept {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return false;
    }
    return blocked[CellIndex(x, y)] == 0U;
}

void TilemapGameplayGrid::CellsToWorldPolylineXY(
        const Array<GridPathfinder::Cell>& cells,
        const Vector2& gridOriginXY,
        const float cellSize,
        Array<Vector2>& outWorldXY) {
    outWorldXY.Clear();
    outWorldXY.Reserve(cells.GetSize());
    for (std::size_t i = 0; i < cells.GetSize(); ++i) {
        const GridPathfinder::Cell& cell = cells[i];
        outWorldXY.PushBack(
                {gridOriginXY.x + (static_cast<float>(cell.x) + 0.5F) * cellSize,
                 gridOriginXY.y + (static_cast<float>(cell.y) + 0.5F) * cellSize});
    }
}

}  // namespace Spark
