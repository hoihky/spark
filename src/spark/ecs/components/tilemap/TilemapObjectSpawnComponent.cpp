#include "spark/ecs/components/tilemap/TilemapObjectSpawnComponent.hpp"

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapObjectQuery.hpp"
#include "spark/scene/tilemap/TilemapObjectSpawnRegistry.hpp"

namespace Spark {

void TilemapObjectSpawnComponent::OnAttach(GameObject& owner) {
    if (!spawnOnAttach) {
        return;
    }
    SpawnFromLayers(owner, owner.GetWorld());
}

void TilemapObjectSpawnComponent::OnDetach(GameObject& owner) {
    ClearSpawned(owner.GetWorld());
}

void TilemapObjectSpawnComponent::ClearSpawned(GameWorld& world) noexcept {
    for (std::size_t i = 0; i < spawned_.GetSize(); ++i) {
        if (spawned_[i] != nullptr) {
            world.DestroyGameObject(spawned_[i]);
        }
    }
    spawned_.Clear();
}

void TilemapObjectSpawnComponent::RespawnAll(GameObject& owner, GameWorld& world) noexcept {
    ClearSpawned(world);
    SpawnFromLayers(owner, world);
}

void TilemapObjectSpawnComponent::SpawnFromLayers(GameObject& owner, GameWorld& world) noexcept {
    const TilemapObjectLayerComponent* objects = owner.GetComponent<TilemapObjectLayerComponent>();
    const TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (objects == nullptr || tilemap == nullptr) {
        return;
    }

    const TilemapGridFrame frame = MakeTilemapGridFrameForObject(owner, *tilemap);
    const Array<TilemapObjectLayer>& layers = objects->GetObjectLayers();
    for (std::size_t li = 0; li < layers.GetSize(); ++li) {
        if (!layers[li].visible) {
            continue;
        }
        const Array<TilemapObjectMarker>& markers = layers[li].markers;
        for (std::size_t mi = 0; mi < markers.GetSize(); ++mi) {
            const TilemapObjectMarker& marker = markers[mi];
            if (marker.mode == TilemapObjectMarkerMode::GizmoOnly) {
                continue;
            }
            if (marker.typeId.IsEmpty()) {
                continue;
            }
            const TilemapObjectSpawnFn spawnFn = TilemapObjectSpawnRegistry::Find(marker.typeId);
            if (spawnFn == nullptr) {
                continue;
            }
            if (GameObject* spawned = spawnFn(world, owner, marker, frame); spawned != nullptr) {
                spawned_.PushBack(spawned);
            }
        }
    }
}

}  // namespace Spark
