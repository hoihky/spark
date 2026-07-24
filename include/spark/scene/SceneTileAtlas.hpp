#pragma once

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <cstdint>

namespace Spark {

/** Maps a tile id into normalized atlas UV bounds (minU, minV, maxU, maxV). */
void TileIdToAtlasUvRect(
        std::uint16_t tileId,
        std::uint32_t atlasTilesU,
        std::uint32_t atlasTilesV,
        Vector4& outUv) noexcept;

/** Atlas UV with optional margin/spacing in texel space (Tiled-style grid). */
void TileIdToAtlasUvRect(
        std::uint16_t tileId,
        std::uint32_t atlasTilesU,
        std::uint32_t atlasTilesV,
        float marginPixels,
        float spacingPixels,
        std::uint32_t textureWidth,
        std::uint32_t textureHeight,
        Vector4& outUv) noexcept;

void TileIdToAtlasUvRect(
        std::uint16_t tileId,
        std::uint32_t atlasTilesU,
        std::uint32_t atlasTilesV,
        float marginPixels,
        float spacingPixels,
        std::uint32_t textureWidth,
        std::uint32_t textureHeight,
        std::uint32_t tilePixelWidth,
        std::uint32_t tilePixelHeight,
        Vector4& outUv) noexcept;

void TileIdToAtlasUvRect(const Tileset& tileset, std::uint16_t tileId, Vector4& outUv) noexcept;

}  // namespace Spark
