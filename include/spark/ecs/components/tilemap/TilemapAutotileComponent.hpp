#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/scene/tilemap/TilemapEditRevision.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class IEngineContext;
class TilemapComponent;

/**
 * Maintains autotiled display tiles on a <c>TilemapComponent</c> layer from painted terrain ids
 * (<c>TileCell::paintTileId</c> / <c>SetPaintTile</c>).
 */
class TilemapAutotileComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapAutotile;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_layer_index)
    [[nodiscard]] std::uint32_t GetLayerIndex() const noexcept { return layerIndex; }
    void SetLayerIndex(const std::uint32_t index) noexcept {
        layerIndex = index;
        RequestRebuild();
    }

    SPARK_SCRIPT_BIND(get_rebuild_on_update)
    [[nodiscard]] bool GetRebuildOnUpdate() const noexcept { return rebuildOnUpdate; }
    SPARK_SCRIPT_BIND(set_rebuild_on_update)
    void SetRebuildOnUpdate(const bool enabled) noexcept { rebuildOnUpdate = enabled; }

    SPARK_SCRIPT_BIND(request_rebuild)
    void RequestRebuild() noexcept { rebuildRequested = true; }

    SPARK_SCRIPT_BIND(rebuild_if_needed)
    void RebuildIfNeeded(GameObject& owner) noexcept;

    /** Partial autotile rebuild inside <c>region</c> (typically margin-expanded). */
    SPARK_SCRIPT_BIND(rebuild_region)
    void RebuildRegion(GameObject& owner, const TilemapCellRegion& region) noexcept;

    SPARK_SCRIPT_BIND(clear_rebuild_request)
    void ClearRebuildRequest() noexcept { rebuildRequested = false; }

    /** Sets painted terrain and rebuilds autotile display for this layer. */
    SPARK_SCRIPT_BIND(paint_tile)
    void PaintTile(GameObject& owner, std::uint32_t x, std::uint32_t y, std::uint16_t paintTileId) noexcept;

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    std::uint32_t layerIndex = 0;
    bool rebuildOnUpdate = false;
    bool rebuildRequested = true;
};

}  // namespace Spark
