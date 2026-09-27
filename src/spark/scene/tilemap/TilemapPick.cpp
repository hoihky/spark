#include "spark/scene/tilemap/TilemapPick.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/scene/tilemap/TilemapLayer.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/TilemapObjectQuery.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] TilemapCellPick MakeCellPick(
        const TilemapComponent& tilemap,
        const std::uint32_t layerIndex,
        const GridPathfinder::Cell& cell,
        const bool requireNonEmpty) noexcept {
    TilemapCellPick result{};
    if (cell.x < 0 || cell.y < 0 || static_cast<std::uint32_t>(cell.x) >= tilemap.GetMapWidth() ||
        static_cast<std::uint32_t>(cell.y) >= tilemap.GetMapHeight()) {
        return result;
    }
    if (layerIndex >= tilemap.GetLayerCount()) {
        return result;
    }
    const TileCell tile = tilemap.GetTileCell(layerIndex, static_cast<std::uint32_t>(cell.x),
            static_cast<std::uint32_t>(cell.y));
    if (requireNonEmpty && !tile.HasVisual()) {
        return result;
    }
    result.hit = true;
    result.layerIndex = layerIndex;
    result.cell = cell;
    result.tile = tile;
    return result;
}

}  // namespace

TilemapCellPick TilemapPicker::PickCellAtWorld(
        const GameObject& owner,
        const TilemapComponent& tilemap,
        const Vector2& worldXY) const noexcept {
    const TilemapGridFrame frame = TilemapGridFrame::FromTilemapObject(owner, tilemap);
    const GridPathfinder::Cell cell = frame.WorldXYToCell(worldXY);
    TilemapCellPick result{};
    if (!frame.IsCellInBounds(cell)) {
        return result;
    }
    result.hit = true;
    result.cell = cell;
    result.layerIndex = 0U;
    if (tilemap.GetLayerCount() > 0U) {
        result.tile = tilemap.GetTileCell(0U, static_cast<std::uint32_t>(cell.x),
                static_cast<std::uint32_t>(cell.y));
    }
    return result;
}

TilemapCellPick TilemapPicker::PickTopmostTile(
        const GameObject& owner,
        const TilemapComponent& tilemap,
        const Vector2& worldXY) const noexcept {
    const TilemapGridFrame frame = TilemapGridFrame::FromTilemapObject(owner, tilemap);
    const GridPathfinder::Cell cell = frame.WorldXYToCell(worldXY);
    if (!frame.IsCellInBounds(cell)) {
        return {};
    }
    for (std::uint32_t li = tilemap.GetLayerCount(); li > 0U; --li) {
        const std::uint32_t layerIndex = li - 1U;
        const TilemapLayer& layer = tilemap.GetLayer(layerIndex);
        if (!layer.visible) {
            continue;
        }
        const TilemapCellPick pick = MakeCellPick(tilemap, layerIndex, cell, true);
        if (pick.IsHit()) {
            return pick;
        }
    }
    return {};
}

TilemapCellPick TilemapPicker::PickCellOnLayer(
        const GameObject& owner,
        const TilemapComponent& tilemap,
        const std::uint32_t layerIndex,
        const Vector2& worldXY) const noexcept {
    const TilemapGridFrame frame = TilemapGridFrame::FromTilemapObject(owner, tilemap);
    const GridPathfinder::Cell cell = frame.WorldXYToCell(worldXY);
    return MakeCellPick(tilemap, layerIndex, cell, false);
}

TilemapObjectPick TilemapPicker::PickNearestObjectMarker(
        const GameObject& owner,
        const TilemapComponent& tilemap,
        const TilemapObjectLayerComponent& objects,
        const Vector2& worldXY,
        const float maxDistanceWorld) const noexcept {
    TilemapObjectPick best{};
    const float maxDistSq = maxDistanceWorld * maxDistanceWorld;
    const TilemapGridFrame frame = TilemapGridFrame::FromTilemapObject(owner, tilemap);
    const TilemapObjectQuery objectQuery{};

    for (std::uint32_t li = objects.GetLayerCount(); li > 0U; --li) {
        const std::uint32_t layerIndex = li - 1U;
        const TilemapObjectLayer& layer = objects.GetLayer(layerIndex);
        if (!layer.visible) {
            continue;
        }
        for (std::size_t mi = 0; mi < layer.markers.GetSize(); ++mi) {
            const TilemapObjectMarker& marker = layer.markers[mi];
            const Vector3 worldPos = objectQuery.MarkerWorldPosition(marker, frame);
            const float dx = worldPos.x - worldXY.x;
            const float dy = worldPos.y - worldXY.y;
            const float distSq = dx * dx + dy * dy;
            if (distSq > maxDistSq) {
                continue;
            }
            if (!best.IsHit() || distSq < best.GetDistanceSquared()) {
                best.hit = true;
                best.objectLayerIndex = layerIndex;
                best.markerId = marker.id;
                best.distanceSquared = distSq;
            }
        }
    }
    return best;
}

}  // namespace Spark
