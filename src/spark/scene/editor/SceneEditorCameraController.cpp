#include "spark/scene/editor/SceneEditorCameraController.hpp"

#include "spark/engine/IInput.hpp"
#include "spark/math/Constants.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

void OrbitFlyCameraAroundPivot(
        FlyCamera& cam,
        const Vector3& pivot,
        float& orbitDistance,
        const float deltaX,
        const float deltaY) noexcept {
    cam.AddLook(deltaX, deltaY);
    orbitDistance = std::max(1.5F, orbitDistance);
    const Vector3 forward = cam.Forward();
    cam.position = {
            pivot.x - forward.x * orbitDistance,
            pivot.y - forward.y * orbitDistance,
            pivot.z - forward.z * orbitDistance};
}

void PanFlyCamera(FlyCamera& cam, const float deltaX, const float deltaY, const float panScale) noexcept {
    const Vector3 forward = cam.Forward();
    Vector3 right = Vector3::Cross(forward, Vector3::UnitY);
    if (right.LengthSquared() < Epsilon) {
        right = Vector3::UnitX;
    } else {
        right = right.Normalized();
    }
    const Vector3 up = Vector3::Cross(right, forward).Normalized();
    cam.position -= right * (deltaX * panScale) + up * (deltaY * panScale);
}

}  // namespace

void SceneEditorCameraController::ResetToDefault() noexcept {
    camera.position = {8.0F, 6.5F, 14.0F};
    orbitPivot = Vector3::Zero;
    camera.SnapLookAt(orbitPivot);
    const Vector3 offset{
            camera.position.x - orbitPivot.x,
            camera.position.y - orbitPivot.y,
            camera.position.z - orbitPivot.z};
    orbitDistance = std::max(3.0F, offset.Length());
}

void SceneEditorCameraController::FocusOn(const Vector3& target) noexcept {
    orbitPivot = target;
    const Vector3 offset{
            camera.position.x - orbitPivot.x,
            camera.position.y - orbitPivot.y,
            camera.position.z - orbitPivot.z};
    orbitDistance = std::max(3.0F, offset.Length());
    camera.SnapLookAt(orbitPivot);
}

void SceneEditorCameraController::UpdateFlyNavigation(IInput& input, const FrameTiming& timing) noexcept {
    if (timing.frameIndex > 0) {
        camera.AddLook(input.GetMouseDeltaX(), input.GetMouseDeltaY());
    }
    camera.ProcessMovement(input, timing.deltaTimeSeconds);
    const float scroll = input.GetScrollDeltaY();
    if (std::fabs(scroll) > 1.0e-4F) {
        camera.position += camera.Forward() * (scroll * 0.65F);
    }
}

void SceneEditorCameraController::UpdateEditorNavigation(
        IInput& input,
        const FrameTiming& timing,
        const bool inViewport,
        const bool uiConsumesPointer) noexcept {
    if (uiConsumesPointer) {
        return;
    }

    const bool rmbDown = input.IsMouseButtonDown(1);
    const bool mmbDown = input.IsMouseButtonDown(2);
    const bool cameraNavActive = inViewport || rmbDown || mmbDown || orbitDragActive;

    if (input.IsMouseButtonPressedThisFrame(1) && inViewport) {
        rmbDragDistSq = 0.0F;
    }
    if (rmbDown && inViewport && timing.frameIndex > 0) {
        const float mdx = input.GetMouseDeltaX();
        const float mdy = input.GetMouseDeltaY();
        rmbDragDistSq += mdx * mdx + mdy * mdy;
        camera.AddLook(mdx, mdy);
    }

    if (mmbDown && inViewport && timing.frameIndex > 0) {
        const float panScale = 0.014F * std::max(1.0F, orbitDistance * 0.08F);
        PanFlyCamera(camera, input.GetMouseDeltaX(), input.GetMouseDeltaY(), panScale);
    }

    if (inViewport && std::fabs(input.GetScrollDeltaY()) > 1.0e-4F) {
        camera.position += camera.Forward() * (input.GetScrollDeltaY() * 0.65F);
    }

    if (cameraNavActive) {
        camera.ProcessMovement(input, timing.deltaTimeSeconds);
    }
}

void SceneEditorCameraController::BeginOrbitDrag(const Vector3& pivot) noexcept {
    orbitPivot = pivot;
    const Vector3 offset{
            camera.position.x - orbitPivot.x,
            camera.position.y - orbitPivot.y,
            camera.position.z - orbitPivot.z};
    orbitDistance = std::max(1.5F, offset.Length());
    orbitDragActive = true;
}

void SceneEditorCameraController::UpdateOrbitDrag(IInput& input) noexcept {
    if (!orbitDragActive) {
        return;
    }
    OrbitFlyCameraAroundPivot(
            camera, orbitPivot, orbitDistance, input.GetMouseDeltaX(), input.GetMouseDeltaY());
}

void SceneEditorCameraController::EndOrbitDrag() noexcept {
    orbitDragActive = false;
}

}  // namespace Spark
