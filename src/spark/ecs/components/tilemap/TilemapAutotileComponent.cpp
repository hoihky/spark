#include "spark/ecs/components/tilemap/TilemapAutotileComponent.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/TileAutotileBake.hpp"

namespace Spark {

void TilemapAutotileComponent::RebuildIfNeeded(GameObject& owner) noexcept {
    if (!rebuildOnUpdate && !rebuildRequested) {
        return;
    }
    TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        rebuildRequested = false;
        return;
    }
    RebuildTilemapAutotileLayer(*tilemap, layerIndex);
    rebuildRequested = false;
}

void TilemapAutotileComponent::PaintTile(
        GameObject& owner,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint16_t paintTileId) noexcept {
    TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (tilemap == nullptr) {
        return;
    }
    tilemap->SetPaintTile(layerIndex, x, y, paintTileId);
    RequestRebuild();
    RebuildIfNeeded(owner);
}

void TilemapAutotileComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& /*context*/) {
    RebuildIfNeeded(owner);
}

}  // namespace Spark
