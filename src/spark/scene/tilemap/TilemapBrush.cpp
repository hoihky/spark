#include "spark/scene/tilemap/TilemapBrush.hpp"

namespace Spark {

namespace {

[[nodiscard]] std::uint32_t HashCell(
        const std::uint32_t cellX,
        const std::uint32_t cellY,
        const std::uint32_t seed) noexcept {
    std::uint32_t h = cellX * 374761393U + cellY * 668265263U + seed * 1442695041U;
    h ^= h >> 13U;
    h *= 1274126177U;
    return h;
}

}  // namespace

TileCell TilemapBrush::ResolveCell(
        const std::uint32_t originCellX,
        const std::uint32_t originCellY,
        const std::uint32_t cellX,
        const std::uint32_t cellY) const noexcept {
    switch (mode) {
        case Mode::Single:
            return single;
        case Mode::TerrainPaint: {
            if (terrainPaintId == TileCell::kEmptyTileId) {
                return TileCell::Empty();
            }
            TileCell cell = TileCell::FromTileId(terrainPaintId);
            cell.paintTileId = terrainPaintId;
            return cell;
        }
        case Mode::RandomPalette: {
            if (paletteTileIds.IsEmpty()) {
                return TileCell::Empty();
            }
            const std::uint32_t index =
                    HashCell(cellX, cellY, randomSeed) % static_cast<std::uint32_t>(paletteTileIds.GetSize());
            return TileCell::FromTileId(paletteTileIds[index]);
        }
        case Mode::Stamp: {
            if (stamp.IsEmpty()) {
                return TileCell::Empty();
            }
            const std::int32_t localX =
                    static_cast<std::int32_t>(cellX) - static_cast<std::int32_t>(originCellX) - stamp.anchorCellX;
            const std::int32_t localY =
                    static_cast<std::int32_t>(cellY) - static_cast<std::int32_t>(originCellY) - stamp.anchorCellY;
            if (localX < 0 || localY < 0 || static_cast<std::uint32_t>(localX) >= stamp.width ||
                static_cast<std::uint32_t>(localY) >= stamp.height) {
                return TileCell::Empty();
            }
            const std::size_t index = static_cast<std::size_t>(localY) * static_cast<std::size_t>(stamp.width) +
                                      static_cast<std::size_t>(localX);
            if (index >= stamp.cells.GetSize()) {
                return TileCell::Empty();
            }
            return stamp.cells[index];
        }
    }
    return TileCell::Empty();
}

}  // namespace Spark
