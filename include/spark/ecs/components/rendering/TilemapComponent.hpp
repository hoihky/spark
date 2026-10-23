#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/tilemap/TileCell.hpp"
#include "spark/scene/tilemap/TilemapLayer.hpp"
#include "spark/scene/tilemap/TilemapGameplayGrid.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Grid of textured quads sharing one tileset atlas. Tiles are row-major in local space: (0,0) at the origin corner,
 * +X and +Y along grid axes; each cell is <c>tileWorldSize</c> world units.
 *
 * Multiple <c>TilemapLayer</c> entries stack draw order via per-layer <c>orderInLayerOffset</c> (plus
 * <c>RenderLayerComponent</c> / sorting groups on the owning object).
 */
class TilemapComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Tilemap;
    static constexpr std::uint16_t kEmptyTile = TileCell::kEmptyTileId;
    static constexpr std::uint32_t kDefaultLayerIndex = 0U;

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

    explicit TilemapComponent(
            SharedPtr<Tileset> inTileset,
            std::uint32_t mapW,
            std::uint32_t mapH,
            float inTileWorldSize,
            std::int32_t inSortOrderBase) noexcept;

    [[nodiscard]] const SharedPtr<Tileset>& GetTileset() const noexcept { return tileset; }
    void SetTileset(SharedPtr<Tileset> inTileset) noexcept;

    [[nodiscard]] const SharedPtr<Texture2D>& GetAtlas() const noexcept;
    SPARK_SCRIPT_BIND(get_map_width)
    [[nodiscard]] std::uint32_t GetMapWidth() const noexcept { return mapWidth; }
    SPARK_SCRIPT_BIND(get_map_height)
    [[nodiscard]] std::uint32_t GetMapHeight() const noexcept { return mapHeight; }
    SPARK_SCRIPT_BIND(get_atlas_tiles_u)
    [[nodiscard]] std::uint32_t GetAtlasTilesU() const noexcept;
    SPARK_SCRIPT_BIND(get_atlas_tiles_v)
    [[nodiscard]] std::uint32_t GetAtlasTilesV() const noexcept;
    SPARK_SCRIPT_BIND(get_tile_world_size)
    [[nodiscard]] float GetTileWorldSize() const noexcept { return tileWorldSize; }
    SPARK_SCRIPT_BIND(set_tile_world_size)
    void SetTileWorldSize(const float size) noexcept { tileWorldSize = size > 0.0F ? size : 1.0F; }
    SPARK_SCRIPT_BIND(get_sort_order_base)
    [[nodiscard]] std::int32_t GetSortOrderBase() const noexcept { return sortOrderBase; }
    SPARK_SCRIPT_BIND(set_sort_order_base)
    void SetSortOrderBase(std::int32_t order) noexcept { sortOrderBase = order; }

    SPARK_SCRIPT_BIND(resize)
    void Resize(std::uint32_t mapW, std::uint32_t mapH);

    [[nodiscard]] std::uint32_t GetLayerCount() const noexcept {
        return static_cast<std::uint32_t>(layers.GetSize());
    }
    SPARK_SCRIPT_BIND(get_layer)
    [[nodiscard]] TilemapLayer& GetLayer(const std::uint32_t layerIndex) noexcept;
    SPARK_SCRIPT_BIND(get_layer)
    [[nodiscard]] const TilemapLayer& GetLayer(const std::uint32_t layerIndex) const noexcept;
    /** Appends an empty layer with the current map size. @return New layer index. */
    SPARK_SCRIPT_BIND(get_add_layer)
    [[nodiscard]] std::uint32_t AddLayer(const char* name = "Layer");
    /** Removes a layer when more than one exists. */
    SPARK_SCRIPT_BIND(remove_layer)
    void RemoveLayer(std::uint32_t layerIndex) noexcept;

    /** Legacy API: layer 0, default transform and full opacity. */
    void SetTile(std::uint32_t x, std::uint32_t y, std::uint16_t tileId);
    /** Returns <c>kEmptyTile</c> for out-of-range or empty cells (layer 0). */
    SPARK_SCRIPT_BIND(get_tile)
    [[nodiscard]] std::uint16_t GetTile(std::uint32_t x, std::uint32_t y) const noexcept;

    SPARK_SCRIPT_BIND(set_tile_cell)
    void SetTileCell(std::uint32_t x, std::uint32_t y, const TileCell& cell);
    SPARK_SCRIPT_BIND(get_tile_cell)
    [[nodiscard]] TileCell GetTileCell(std::uint32_t x, std::uint32_t y) const noexcept;

    void SetTile(std::uint32_t layerIndex, std::uint32_t x, std::uint32_t y, std::uint16_t tileId);
    SPARK_SCRIPT_BIND(set_tile_cell)
    void SetTileCell(std::uint32_t layerIndex, std::uint32_t x, std::uint32_t y, const TileCell& cell);
    SPARK_SCRIPT_BIND(get_tile_cell)
    [[nodiscard]] TileCell GetTileCell(std::uint32_t layerIndex, std::uint32_t x, std::uint32_t y) const noexcept;

    SPARK_SCRIPT_BIND(get_tile_definition)
    [[nodiscard]] const TileDefinition& GetTileDefinition(std::uint32_t x, std::uint32_t y) const noexcept;
    [[nodiscard]] const TileDefinition& GetTileDefinition(
            std::uint32_t layerIndex,
            std::uint32_t x,
            std::uint32_t y) const noexcept;

    /** Definition for the painted / terrain id (autotile source), not the display tile. */
    [[nodiscard]] const TileDefinition& GetPaintTileDefinition(
            std::uint32_t layerIndex,
            std::uint32_t x,
            std::uint32_t y) const noexcept;

    SPARK_SCRIPT_BIND(get_definition_for_tile_id)
    [[nodiscard]] const TileDefinition& GetDefinitionForTileId(std::uint16_t tileId) const noexcept;

    /** Sets terrain paint id and display id; use with <c>TilemapAutotileComponent</c>. */
    SPARK_SCRIPT_BIND(set_paint_tile)
    void SetPaintTile(std::uint32_t x, std::uint32_t y, std::uint16_t paintTileId);
    SPARK_SCRIPT_BIND(set_paint_tile)
    void SetPaintTile(std::uint32_t layerIndex, std::uint32_t x, std::uint32_t y, std::uint16_t paintTileId);

    SPARK_SCRIPT_BIND(bake_gameplay_grid)
    void BakeGameplayGrid(TilemapGameplayGrid& outGrid, TilemapGameplayWalkRule rule) const noexcept;

private:
    void EnsureDefaultLayer() noexcept;
    [[nodiscard]] bool TryCellIndex(
            std::uint32_t layerIndex,
            std::uint32_t x,
            std::uint32_t y,
            std::size_t& outIndex) const noexcept;

    SharedPtr<Tileset> tileset{};
    std::uint32_t mapWidth = 0;
    std::uint32_t mapHeight = 0;
    float tileWorldSize = 1.0F;
    std::int32_t sortOrderBase = 0;
    Array<TilemapLayer> layers{};
};

}  // namespace Spark
