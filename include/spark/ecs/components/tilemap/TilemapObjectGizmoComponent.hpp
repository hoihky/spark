#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"

#include "spark/scene/tilemap/TilemapObject.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class IEngineContext;

/**
 * Debug visualization for map object markers (F3). Draws gizmo-only markers by default;
 * enable <c>drawRuntimeMarkers</c> to show spawn points before/without spawning.
 */
class TilemapObjectGizmoComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapObjectGizmo;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] bool GetDrawGizmos() const noexcept { return drawGizmos; }
    void SetDrawGizmos(const bool enabled) noexcept {
        drawGizmos = enabled;
        visualsDirty_ = true;
    }

    /** When true, also draws non-gizmo markers (useful in editor-style views). */
    [[nodiscard]] bool GetDrawRuntimeMarkers() const noexcept { return drawRuntimeMarkers; }
    void SetDrawRuntimeMarkers(const bool enabled) noexcept {
        drawRuntimeMarkers = enabled;
        visualsDirty_ = true;
    }

    void SetGizmoTexture(SharedPtr<Texture2D> texture) noexcept {
        gizmoTexture = MoveTemp(texture);
        visualsDirty_ = true;
    }

    void SetGizmoUvRect(const Vector4& uv) noexcept {
        gizmoUv = uv;
        visualsDirty_ = true;
    }

    void SetGizmoTint(const Vector4& rgba) noexcept { gizmoTint = rgba; }

    void RebuildVisuals(GameObject& owner, GameWorld& world) noexcept;

    void OnAttach(GameObject& owner) override;
    void OnDetach(GameObject& owner) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

private:
    void ClearVisuals(GameWorld& world) noexcept;
    bool ShouldDrawMarker(const TilemapObjectMarker& marker) const noexcept;

    SharedPtr<Texture2D> gizmoTexture{};
    Vector4 gizmoUv{0.0F, 0.0F, 1.0F, 1.0F};
    Vector4 gizmoTint{0.95F, 0.35F, 0.95F, 0.75F};
    float gizmoScale = 0.45F;
    std::int32_t gizmoSortOrder = 45;
    bool drawGizmos = true;
    bool drawRuntimeMarkers = false;
    bool visualsDirty_ = true;
    Array<GameObject*> gizmoObjects_{};
};

}  // namespace Spark
