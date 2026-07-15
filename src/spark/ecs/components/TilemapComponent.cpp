#include "spark/ecs/components/TilemapComponent.hpp"

namespace Spark {

TilemapComponent::TilemapComponent(
        SharedPtr<Texture2D> inAtlas,
        std::uint32_t mapW,
        std::uint32_t mapH,
        std::uint32_t inAtlasTilesU,
        std::uint32_t inAtlasTilesV,
        float inTileWorldSize,
        std::int32_t inSortOrderBase) noexcept
        : atlas(MoveTemp(inAtlas)),
          atlasTilesU(inAtlasTilesU > 0 ? inAtlasTilesU : 1U),
          atlasTilesV(inAtlasTilesV > 0 ? inAtlasTilesV : 1U),
          tileWorldSize(inTileWorldSize > 0.0F ? inTileWorldSize : 1.0F),
          sortOrderBase(inSortOrderBase) {
    Resize(mapW, mapH);
}

void TilemapComponent::Resize(std::uint32_t mapW, std::uint32_t mapH) {
    mapWidth = mapW;
    mapHeight = mapH;
    tiles.Clear();
    const std::uint64_t n = static_cast<std::uint64_t>(mapW) * static_cast<std::uint64_t>(mapH);
    if (n == 0 || n > 1ULL << 24) {
        return;
    }
    tiles.Resize(static_cast<std::size_t>(n));
    for (std::size_t i = 0; i < tiles.GetSize(); ++i) {
        tiles[i] = kEmptyTile;
    }
}

void TilemapComponent::SetTile(std::uint32_t x, std::uint32_t y, std::uint16_t tileId) {
    if (x >= mapWidth || y >= mapHeight) {
        return;
    }
    tiles[y * mapWidth + x] = tileId;
}

std::uint16_t TilemapComponent::GetTile(std::uint32_t x, std::uint32_t y) const noexcept {
    if (x >= mapWidth || y >= mapHeight) {
        return kEmptyTile;
    }
    return tiles[y * mapWidth + x];
}

}  // namespace Spark
