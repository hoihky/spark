#include "spark/gameplay/fog/FogOfWarExplorationMap.hpp"

#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

#include <cmath>

namespace Spark {

void FogOfWarExplorationMap::Reset(const std::int32_t width, const std::int32_t height) noexcept {
    mapWidth = width > 0 ? width : 0;
    mapHeight = height > 0 ? height : 0;
    const std::size_t total =
            static_cast<std::size_t>(mapWidth) * static_cast<std::size_t>(mapHeight);
    cells.Resize(total);
    for (std::size_t i = 0; i < total; ++i) {
        cells[i] = FogOfWarCellState::Hidden;
    }
}

FogOfWarCellState FogOfWarExplorationMap::GetCellState(const std::int32_t x, const std::int32_t y) const noexcept {
    if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) {
        return FogOfWarCellState::Hidden;
    }
    const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(mapWidth) +
            static_cast<std::size_t>(x);
    return cells[index];
}

void FogOfWarExplorationMap::SetCellState(
        const std::int32_t x,
        const std::int32_t y,
        const FogOfWarCellState state) noexcept {
    if (x < 0 || y < 0 || x >= mapWidth || y >= mapHeight) {
        return;
    }
    const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(mapWidth) +
            static_cast<std::size_t>(x);
    cells[index] = state;
}

void FogOfWarExplorationMap::RevealDisc(
        const Vector2& worldCenter,
        const float worldRadius,
        const TilemapGridFrame& frame) noexcept {
    if (mapWidth <= 0 || mapHeight <= 0 || worldRadius <= 0.0F) {
        return;
    }
    const GridPathfinder::Cell center = frame.WorldXYToCell(worldCenter);
    const float radiusCells = worldRadius / std::max(frame.cellSize, 0.001F);
    const int extent = static_cast<int>(std::ceil(radiusCells)) + 1;
    const float radiusSq = worldRadius * worldRadius;

    for (std::int32_t y = 0; y < mapHeight; ++y) {
        for (std::int32_t x = 0; x < mapWidth; ++x) {
            const std::size_t index = static_cast<std::size_t>(y) * static_cast<std::size_t>(mapWidth) +
                    static_cast<std::size_t>(x);
            bool inDisc = false;
            const int dx = x - center.x;
            const int dy = y - center.y;
            if (dx >= -extent && dx <= extent && dy >= -extent && dy <= extent) {
                const Vector2 cellCenter = frame.CellCenterToWorldXY({x, y});
                const float distSq =
                        (cellCenter.x - worldCenter.x) * (cellCenter.x - worldCenter.x) +
                        (cellCenter.y - worldCenter.y) * (cellCenter.y - worldCenter.y);
                inDisc = distSq <= radiusSq;
            }
            if (inDisc) {
                cells[index] = FogOfWarCellState::Visible;
            } else if (cells[index] == FogOfWarCellState::Visible) {
                cells[index] = FogOfWarCellState::Explored;
            }
        }
    }
}

}  // namespace Spark
