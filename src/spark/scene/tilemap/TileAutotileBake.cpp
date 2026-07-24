#include "spark/scene/tilemap/TileAutotileBake.hpp"

namespace Spark {

namespace {

[[nodiscard]] std::uint8_t AutotileGroupForPaintId(const TilemapComponent& tilemap, const std::uint16_t paintId) noexcept {
    if (paintId == TileCell::kEmptyTileId) {
        return 0;
    }
    const SharedPtr<Tileset>& tileset = tilemap.GetTileset();
    if (!tileset) {
        return 0;
    }
    return tileset->Definition(paintId).autotileGroup;
}

[[nodiscard]] bool CellParticipatesInAutotileGroup(
        const TilemapComponent& tilemap,
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint8_t groupId) noexcept {
    const TileCell cell = tilemap.GetTileCell(layerIndex, x, y);
    const std::uint16_t paintId = cell.GetPaintTileId();
    if (paintId == TileCell::kEmptyTileId) {
        return false;
    }
    return AutotileGroupForPaintId(tilemap, paintId) == groupId;
}

[[nodiscard]] std::uint8_t ComputeNeighborMask4(
        const TilemapComponent& tilemap,
        const std::uint32_t layerIndex,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint8_t groupId) noexcept {
    std::uint8_t mask = 0;
    if (y + 1U < tilemap.GetMapHeight() &&
        CellParticipatesInAutotileGroup(tilemap, layerIndex, x, y + 1U, groupId)) {
        mask |= 1U;
    }
    if (x + 1U < tilemap.GetMapWidth() &&
        CellParticipatesInAutotileGroup(tilemap, layerIndex, x + 1U, y, groupId)) {
        mask |= 2U;
    }
    if (y > 0 && CellParticipatesInAutotileGroup(tilemap, layerIndex, x, y - 1U, groupId)) {
        mask |= 4U;
    }
    if (x > 0 && CellParticipatesInAutotileGroup(tilemap, layerIndex, x - 1U, y, groupId)) {
        mask |= 8U;
    }
    return mask;
}

}  // namespace

void RebuildTilemapAutotileLayer(TilemapComponent& tilemap, const std::uint32_t layerIndex) noexcept {
    if (layerIndex >= tilemap.GetLayerCount()) {
        return;
    }
    const SharedPtr<Tileset>& tilesetPtr = tilemap.GetTileset();
    if (!tilesetPtr) {
        return;
    }
    const Tileset& tileset = *tilesetPtr;
    const std::uint32_t mw = tilemap.GetMapWidth();
    const std::uint32_t mh = tilemap.GetMapHeight();
    if (mw == 0 || mh == 0) {
        return;
    }

    for (std::uint32_t y = 0; y < mh; ++y) {
        for (std::uint32_t x = 0; x < mw; ++x) {
            TileCell cell = tilemap.GetTileCell(layerIndex, x, y);
            const std::uint16_t paintId = cell.GetPaintTileId();
            if (paintId == TileCell::kEmptyTileId) {
                continue;
            }
            const std::uint8_t group = tileset.Definition(paintId).autotileGroup;
            if (group == 0) {
                continue;
            }
            const TileAutotileRuleSet* rules = tileset.FindAutotileRuleSet(group);
            if (rules == nullptr) {
                continue;
            }
            const std::uint8_t mask = ComputeNeighborMask4(tilemap, layerIndex, x, y, group);
            const TileAutotileVariant& variant = rules->variants[mask];
            if (variant.tileId != 0) {
                cell.tileId = variant.tileId;
            } else {
                cell.tileId = paintId;
            }
            cell.transformFlags = variant.transformFlags;
            tilemap.SetTileCell(layerIndex, x, y, cell);
        }
    }
}

}  // namespace Spark
