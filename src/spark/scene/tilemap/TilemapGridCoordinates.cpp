#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

#include <cmath>

namespace Spark {

Vector2 TilemapGridFrame::LocalCornerToWorldXY() const noexcept {
    const Vector3 corner = worldFromLocal.TransformPoint({0.0F, 0.0F, 0.0F});
    return {corner.x, corner.y};
}

Vector2 TilemapGridFrame::CellCenterToWorldXY(const GridPathfinder::Cell& cell) const noexcept {
    const Vector3 local{
            (static_cast<float>(cell.x) + 0.5F) * cellSize,
            (static_cast<float>(cell.y) + 0.5F) * cellSize,
            0.0F};
    const Vector3 world = worldFromLocal.TransformPoint(local);
    return {world.x, world.y};
}

GridPathfinder::Cell TilemapGridFrame::WorldXYToCell(const Vector2& worldXY) const noexcept {
    Matrix4 localFromWorld{};
    if (!worldFromLocal.TryInvert(localFromWorld)) {
        return {};
    }
    const Vector3 local = localFromWorld.TransformPoint({worldXY.x, worldXY.y, 0.0F});
    GridPathfinder::Cell cell{};
    cell.x = static_cast<std::int32_t>(std::floor(local.x / cellSize));
    cell.y = static_cast<std::int32_t>(std::floor(local.y / cellSize));
    return cell;
}

GridPathfinder::Cell TilemapGridFrame::WorldPositionToCell(const Vector3& worldPosition) const noexcept {
    return WorldXYToCell({worldPosition.x, worldPosition.y});
}

bool TilemapGridFrame::IsCellInBounds(const GridPathfinder::Cell& cell) const noexcept {
    return cell.x >= 0 && cell.y >= 0 && static_cast<std::uint32_t>(cell.x) < mapWidth &&
           static_cast<std::uint32_t>(cell.y) < mapHeight;
}

TilemapGridFrame MakeTilemapGridFrame(
        const Matrix4& tilemapWorldMatrix,
        const float tileWorldSize,
        const std::uint32_t mapWidth,
        const std::uint32_t mapHeight) noexcept {
    TilemapGridFrame frame{};
    frame.worldFromLocal = tilemapWorldMatrix;
    frame.cellSize = tileWorldSize > 0.0F ? tileWorldSize : 1.0F;
    frame.mapWidth = mapWidth;
    frame.mapHeight = mapHeight;
    return frame;
}

}  // namespace Spark
