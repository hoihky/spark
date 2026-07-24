#include "spark/ecs/components/tilemap/TilemapObjectGizmoComponent.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapObject.hpp"
#include "spark/scene/tilemap/TilemapObjectQuery.hpp"

namespace Spark {

bool TilemapObjectGizmoComponent::ShouldDrawMarker(const TilemapObjectMarker& marker) const noexcept {
    if (marker.mode == TilemapObjectMarkerMode::GizmoOnly) {
        return true;
    }
    return drawRuntimeMarkers;
}

void TilemapObjectGizmoComponent::ClearVisuals(GameWorld& world) noexcept {
    for (std::size_t i = 0; i < gizmoObjects_.GetSize(); ++i) {
        if (gizmoObjects_[i] != nullptr) {
            world.DestroyGameObject(gizmoObjects_[i]);
        }
    }
    gizmoObjects_.Clear();
}

void TilemapObjectGizmoComponent::RebuildVisuals(GameObject& owner, GameWorld& world) noexcept {
    ClearVisuals(world);
    visualsDirty_ = false;
    if (!drawGizmos || !gizmoTexture) {
        return;
    }

    const TilemapObjectLayerComponent* objects = owner.GetComponent<TilemapObjectLayerComponent>();
    const TilemapComponent* tilemap = owner.GetComponent<TilemapComponent>();
    if (objects == nullptr || tilemap == nullptr) {
        return;
    }

    const TilemapGridFrame frame = MakeTilemapGridFrameForObject(owner, *tilemap);
    const float cellSize = tilemap->GetTileWorldSize();
    const Array<TilemapObjectLayer>& layers = objects->GetObjectLayers();
    for (std::size_t li = 0; li < layers.GetSize(); ++li) {
        if (!layers[li].visible) {
            continue;
        }
        const Array<TilemapObjectMarker>& markers = layers[li].markers;
        for (std::size_t mi = 0; mi < markers.GetSize(); ++mi) {
            const TilemapObjectMarker& marker = markers[mi];
            if (!ShouldDrawMarker(marker)) {
                continue;
            }
            const Vector3 worldPos = TilemapObjectMarkerWorldPosition(marker, frame);
            GameObject* gizmoGo = world.CreateGameObject();
            if (gizmoGo == nullptr) {
                continue;
            }
            gizmoGo->GetName() = marker.name.IsEmpty() ? Utf8String("TilemapObjectGizmo") : marker.name;
            if (TransformComponent* tr = gizmoGo->AddComponent<TransformComponent>()) {
                tr->SetTranslation({worldPos.x, worldPos.y, 0.12F});
                tr->SetUniformScale(cellSize * gizmoScale);
            }
            gizmoGo->AddComponent<SpriteComponent>(gizmoTexture, gizmoTint, gizmoUv, gizmoSortOrder);
            gizmoObjects_.PushBack(gizmoGo);
        }
    }
}

void TilemapObjectGizmoComponent::OnAttach(GameObject& owner) {
    visualsDirty_ = true;
    if (drawGizmos) {
        RebuildVisuals(owner, owner.GetWorld());
    }
}

void TilemapObjectGizmoComponent::OnDetach(GameObject& owner) {
    ClearVisuals(owner.GetWorld());
}

void TilemapObjectGizmoComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (!visualsDirty_) {
        return;
    }
    RebuildVisuals(owner, owner.GetWorld());
}

}  // namespace Spark
