#include "spark/scene/tilemap/TilemapGameplayGridBake.hpp"

#include "spark/scene/tilemap/TilemapGameplayRules.hpp"

namespace Spark {

namespace {

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
        if (TileBlocksGameplayPath(paintAsCell, definition, rule)) {
            return false;
        }
    }
    return anyOccupied;
}

}  // namespace

void BakeTilemapGameplayGrid(
        const TilemapComponent& tilemap,
        const TilemapGameplayWalkRule rule,
        TilemapGameplayGrid& outGrid) {
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
