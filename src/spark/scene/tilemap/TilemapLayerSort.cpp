#include "spark/scene/tilemap/TilemapLayerSort.hpp"

#include "spark/math/Vector4.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] float TileInstanceWorldSortY(
        const Matrix4& world,
        const float tileWorldSize,
        const SceneTilemapTileInstance& tile) noexcept {
    const float lx = (static_cast<float>(tile.gridX) + tile.anchorNormX) * tileWorldSize;
    const float ly = (static_cast<float>(tile.gridY) + tile.anchorNormY) * tileWorldSize;
    const Vector4 wp = world * Vector4(lx, ly, 0.0F, 1.0F);
    const float w = (std::fabs(wp.w) < 1.0e-8F) ? 1.0F : wp.w;
    return wp.y / w;
}

[[nodiscard]] bool TileInstanceDrawnBefore(
        const SceneTilemapTileInstance& a,
        const SceneTilemapTileInstance& b,
        const Matrix4& world,
        const float tileWorldSize) noexcept {
    const float ya = TileInstanceWorldSortY(world, tileWorldSize, a);
    const float yb = TileInstanceWorldSortY(world, tileWorldSize, b);
    if (ya != yb) {
        return ya < yb;
    }
    if (a.gridY != b.gridY) {
        return a.gridY < b.gridY;
    }
    return a.gridX < b.gridX;
}

}  // namespace

void StableSortTilemapTileInstances(
        Array<SceneTilemapTileInstance>& tiles,
        const std::uint32_t begin,
        const std::uint32_t count,
        const TilemapLayerSortMode mode,
        const Matrix4& worldTransform,
        const float tileWorldSize) noexcept {
    if (mode != TilemapLayerSortMode::WorldY || count <= 1U || tileWorldSize <= 0.0F) {
        return;
    }
    const std::size_t beginIndex = static_cast<std::size_t>(begin);
    const std::size_t endIndex = beginIndex + static_cast<std::size_t>(count);
    if (endIndex > tiles.GetSize()) {
        return;
    }

    for (std::size_t i = beginIndex + 1; i < endIndex; ++i) {
        SceneTilemapTileInstance key = tiles[i];
        std::size_t j = i;
        while (j > beginIndex &&
               TileInstanceDrawnBefore(key, tiles[j - 1], worldTransform, tileWorldSize)) {
            tiles[j] = tiles[j - 1];
            --j;
        }
        tiles[j] = key;
    }
}

}  // namespace Spark
