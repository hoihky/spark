#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"

#include <cstdint>

namespace Spark {

/**
 * Grid of textured quads sharing one atlas. Tiles are row-major in local space: (0,0) at the origin corner,
 * +X and +Y along grid axes; each cell is tileWorldSize world units. Empty cells use tile id 0xFFFF.
 */
class TilemapComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Tilemap;
    static constexpr std::uint16_t kEmptyTile = 0xFFFF;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    TilemapComponent() = default;
    TilemapComponent(
            SharedPtr<Texture2D> inAtlas,
            std::uint32_t mapW,
            std::uint32_t mapH,
            std::uint32_t atlasTilesU,
            std::uint32_t atlasTilesV,
            float inTileWorldSize,
            std::int32_t inSortOrderBase) noexcept;

    [[nodiscard]] const SharedPtr<Texture2D>& GetAtlas() const noexcept { return atlas; }
    [[nodiscard]] std::uint32_t GetMapWidth() const noexcept { return mapWidth; }
    [[nodiscard]] std::uint32_t GetMapHeight() const noexcept { return mapHeight; }
    [[nodiscard]] std::uint32_t GetAtlasTilesU() const noexcept { return atlasTilesU; }
    [[nodiscard]] std::uint32_t GetAtlasTilesV() const noexcept { return atlasTilesV; }
    [[nodiscard]] float GetTileWorldSize() const noexcept { return tileWorldSize; }
    [[nodiscard]] std::int32_t GetSortOrderBase() const noexcept { return sortOrderBase; }
    void SetSortOrderBase(std::int32_t order) noexcept { sortOrderBase = order; }

    void Resize(std::uint32_t mapW, std::uint32_t mapH);
    void SetTile(std::uint32_t x, std::uint32_t y, std::uint16_t tileId);
    [[nodiscard]] std::uint16_t GetTile(std::uint32_t x, std::uint32_t y) const noexcept;

private:
    SharedPtr<Texture2D> atlas{};
    std::uint32_t mapWidth = 0;
    std::uint32_t mapHeight = 0;
    std::uint32_t atlasTilesU = 1;
    std::uint32_t atlasTilesV = 1;
    float tileWorldSize = 1.0F;
    std::int32_t sortOrderBase = 0;
    Array<std::uint16_t> tiles{};
};

}  // namespace Spark
