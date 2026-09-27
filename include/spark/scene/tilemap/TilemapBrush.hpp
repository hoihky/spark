#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/tilemap/TileCell.hpp"

#include <cstdint>

namespace Spark {

/** Editor brush: single tile, stamp, palette noise, or terrain paint id. */
class TilemapBrush final {
public:
    class StampLayout {
    public:
        std::int32_t anchorCellX = 0;
        std::int32_t anchorCellY = 0;
        std::uint32_t width = 0U;
        std::uint32_t height = 0U;
        Array<TileCell> cells{};

        [[nodiscard]] bool IsEmpty() const noexcept { return width == 0U || height == 0U; }
    };

    enum class Mode : std::uint8_t {
        Single = 0,
        Stamp = 1,
        RandomPalette = 2,
        TerrainPaint = 3,
    };

    Mode mode = Mode::Single;
    TileCell single = TileCell::Empty();
    StampLayout stamp{};
    Array<std::uint16_t> paletteTileIds{};
    std::uint16_t terrainPaintId = 0U;
    std::uint32_t randomSeed = 1U;

    [[nodiscard]] TileCell ResolveCell(
            std::uint32_t originCellX,
            std::uint32_t originCellY,
            std::uint32_t cellX,
            std::uint32_t cellY) const noexcept;
};

}  // namespace Spark
