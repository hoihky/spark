#include "spark/scene/tilemap/TilemapGameplayGridBake.hpp"

#include "spark/scene/tilemap/TilemapGameplayRules.hpp"

#include <algorithm>

namespace Spark {

namespace {

const TilemapGameplayRuleEvaluator kRules{};

[[nodiscard]] bool IsMapCellWalkable(
        const TilemapComponent& tilemap,
        const TilemapGameplayWalkRule rule,
        const std::uint32_t x,
        const std::uint32_t y) noexcept {
    bool anyOccupied = false;
    for (std::uint32_t layerIndex = 0; layerIndex < tilemap.GetLayerCount(); ++layerIndex) {
        const TilemapLayer& layer = tilemap.GetLayer(layerIndex);
        if (!layer.contributeGameplayGrid) {
            continue;
        }
        const TileCell cell = tilemap.GetTileCell(layerIndex, x, y);
        const std::uint16_t paintId = cell.GetPaintTileId();
        if (paintId == TileCell::kEmptyTileId) {
            continue;
        }
        anyOccupied = true;
        const TileDefinition& definition = tilemap.GetDefinitionForTileId(paintId);
        TileCell paintAsCell = cell;
        paintAsCell.tileId = paintId;
        if (kRules.BlocksGameplayPath(paintAsCell, definition, rule)) {
            return false;
        }
    }
    return anyOccupied;
}

}  // namespace

void TilemapGameplayGridBaker::BakeRegion(
        const TilemapComponent& tilemap,
        const TilemapGameplayWalkRule rule,
        TilemapGameplayGrid& outGrid,
        const TilemapCellRegion& region) const {
    const std::int32_t w = static_cast<std::int32_t>(tilemap.GetMapWidth());
    const std::int32_t h = static_cast<std::int32_t>(tilemap.GetMapHeight());
    if (w <= 0 || h <= 0 || region.IsEmpty()) {
        return;
    }
    if (outGrid.Width() != w || outGrid.Height() != h) {
        BakeFull(tilemap, rule, outGrid);
        return;
    }
    const std::uint32_t maxX = std::min(region.maxX, tilemap.GetMapWidth() - 1U);
    const std::uint32_t maxY = std::min(region.maxY, tilemap.GetMapHeight() - 1U);
    for (std::uint32_t y = region.minY; y <= maxY; ++y) {
        for (std::uint32_t x = region.minX; x <= maxX; ++x) {
            const bool walkable = IsMapCellWalkable(tilemap, rule, x, y);
            outGrid.SetBlocked(static_cast<std::int32_t>(x), static_cast<std::int32_t>(y), !walkable);
        }
    }
}

void TilemapGameplayGridBaker::BakeFull(
        const TilemapComponent& tilemap,
        const TilemapGameplayWalkRule rule,
        TilemapGameplayGrid& outGrid) const {
    const std::int32_t w = static_cast<std::int32_t>(tilemap.GetMapWidth());
    const std::int32_t h = static_cast<std::int32_t>(tilemap.GetMapHeight());
    outGrid.Resize(w, h);
    if (w <= 0 || h <= 0) {
        return;
    }
    for (std::int32_t y = 0; y < h; ++y) {
        for (std::int32_t x = 0; x < w; ++x) {
            const bool walkable =
                    IsMapCellWalkable(tilemap, rule, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
            outGrid.SetBlocked(x, y, !walkable);
        }
    }
}

}  // namespace Spark
