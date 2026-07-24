#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/scene/tilemap/TileCell.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"

#include <cstdint>

namespace Spark {

/** Custom properties on map, tile layer, or object marker (Tiled / LDtk-style). */
using TilemapPropertyList = Array<TilemapObjectProperty>;

/** Resolved tileset used by a document (single-atlas maps; multiple entries for GID lookup). */
struct TilemapDocumentTileset {
    Utf8String name{};
    Utf8String imagePath{};
    std::uint32_t firstGid = 1U;
    std::uint32_t tileWidth = 16U;
    std::uint32_t tileHeight = 16U;
    std::uint32_t spacing = 0U;
    std::uint32_t margin = 0U;
    std::uint32_t columns = 1U;
    std::uint32_t tileCount = 0U;
    /** Pixel size of the tileset image (from Tiled <image width/height>). */
    std::uint32_t imageWidth = 0U;
    std::uint32_t imageHeight = 0U;
    TilemapPropertyList properties{};
};

struct TilemapDocumentTileLayer {
    Utf8String name{};
    bool visible = true;
    std::int32_t orderInLayerOffset = 0;
    bool contributeCollision = true;
    bool contributeGameplayGrid = true;
    Array<TileCell> cells{};
    TilemapPropertyList properties{};
};

/**
 * Portable tilemap description (import/export). Independent of ECS; applied via
 * <c>ApplyTilemapDocument</c> or saved as <c>.sparkmap</c>.
 */
struct TilemapDocument {
    static constexpr const char* kMagic = "spark_tilemap_v1";

    Utf8String sourceTmxPath{};
    std::uint32_t mapWidth = 0U;
    std::uint32_t mapHeight = 0U;
    /** World units per map tile (often <c>tilePixelSize / pixelsPerUnit</c>). */
    float tileWorldSize = 1.0F;
    std::int32_t sortOrderBase = 0;
    TilemapPropertyList mapProperties{};
    Array<TilemapDocumentTileset> tilesets{};
    Array<TilemapDocumentTileLayer> tileLayers{};
    Array<TilemapObjectLayer> objectLayers{};
};

}  // namespace Spark
