#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"

#include <cstdint>

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

    [[nodiscard]] std::uint32_t GetLayerIndex() const noexcept { return layerIndex; }
    void SetLayerIndex(const std::uint32_t index) noexcept {
        layerIndex = index;
        RequestRebuild();
    }

    [[nodiscard]] bool GetRebuildOnUpdate() const noexcept { return rebuildOnUpdate; }
    void SetRebuildOnUpdate(const bool enabled) noexcept { rebuildOnUpdate = enabled; }

    void RequestRebuild() noexcept { rebuildRequested = true; }

    void RebuildIfNeeded(GameObject& owner) noexcept;

    /** Sets painted terrain and rebuilds autotile display for this layer. */
    void PaintTile(GameObject& owner, std::uint32_t x, std::uint32_t y, std::uint16_t paintTileId) noexcept;

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    std::uint32_t layerIndex = 0;
    bool rebuildOnUpdate = false;
    bool rebuildRequested = true;
};

}  // namespace Spark
