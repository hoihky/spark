#pragma once

#include "spark/engine/FrameTiming.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/camera/FlyCamera.hpp"

namespace Spark {

class IInput;

/**
 * Editor viewport camera: orbit, pan, fly, focus, and dolly.
 * Extracted from the scene-editor demo so navigation policy stays testable and reusable.
 */
class SceneEditorCameraController final {
public:
    FlyCamera camera{};
    Vector3 orbitPivot{Vector3::Zero};
    float orbitDistance = 18.0F;

    void ResetToDefault() noexcept;
    void FocusOn(const Vector3& target) noexcept;

    /** Updates look / pan / scroll / WASD when not in fly-capture mode. */
    void UpdateEditorNavigation(
            IInput& input,
            const FrameTiming& timing,
            bool inViewport,
            bool uiConsumesPointer) noexcept;

    /** Fly mode when cursor is captured (F1). */
    void UpdateFlyNavigation(IInput& input, const FrameTiming& timing) noexcept;

    void BeginOrbitDrag(const Vector3& pivot) noexcept;
    void UpdateOrbitDrag(IInput& input) noexcept;
    void EndOrbitDrag() noexcept;
    [[nodiscard]] bool IsOrbitDragging() const noexcept { return orbitDragActive; }

private:
    bool orbitDragActive = false;
    float rmbDragDistSq = 0.0F;
};

}  // namespace Spark
