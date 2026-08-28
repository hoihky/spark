#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/engine/SceneRenderParams.hpp"

namespace Spark {

class TransformComponent;

/** Manipulator mode for the scene-editor transform gizmo (Strategy family). */
enum class TransformGizmoMode {
    Translate,
    Rotate,
    Scale,
};

/**
 * Interactive translate / rotate / scale gizmo for a selected object.
 * Picking, drag math, and debug draws are encapsulated here so the demo stays thin.
 */
class TransformGizmo final {
public:
    void SetMode(TransformGizmoMode mode) noexcept { mode_ = mode; }
    [[nodiscard]] TransformGizmoMode GetMode() const noexcept { return mode_; }
    void CycleMode() noexcept;

    [[nodiscard]] bool IsDragging() const noexcept { return drag_.active; }
    [[nodiscard]] int GetActiveAxis() const noexcept { return drag_.activeAxis; }

    /** Returns true when a gizmo handle consumed the press. */
    [[nodiscard]] bool TryBeginDrag(
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            const TransformComponent& transform,
            float objectExtent,
            GameObject& target) noexcept;

    /** Applies drag delta to <c>transform</c> while LMB is held. */
    [[nodiscard]] bool UpdateDrag(
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            TransformComponent& transform) noexcept;

    void EndDrag() noexcept;

    void AppendDraws(
            const TransformComponent& transform,
            float objectExtent,
            Array<SceneDrawItem>& out) const;

    [[nodiscard]] static const char* ModeName(TransformGizmoMode mode) noexcept;
    [[nodiscard]] const char* GetModeName() const noexcept { return ModeName(mode_); }
    [[nodiscard]] const char* GetInteractionHint() const noexcept;

private:
    struct DragState {
        bool active = false;
        int activeAxis = -1;
        GameObject* target = nullptr;
        Vector3 pivot{Vector3::Zero};
        Vector3 startTranslation{Vector3::Zero};
        Quaternion startRotation = Quaternion::Identity;
        Vector3 startScale{Vector3::One};
        float startLineS = 0.0F;
        float startPlaneAngle = 0.0F;
    };

    TransformGizmoMode mode_ = TransformGizmoMode::Translate;
    DragState drag_{};
};

}  // namespace Spark
