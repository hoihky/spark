#include "spark/editor/EditorViewport.hpp"

#include "spark/editor/EditorCommandStack.hpp"
#include "spark/editor/commands/SetTransformCommand.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"
#include "spark/scene/query/SceneRaycast.hpp"
#include "spark/ui/runtime/UiScene.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

namespace Spark::Editor {

namespace {

bool ScreenToWorldRay(
        const Ui::Rect& viewport,
        const float px,
        const float py,
        const Matrix4& invViewProj,
        Vector3& outOrigin,
        Vector3& outDir) noexcept {
    if (viewport.width <= 0.0F || viewport.height <= 0.0F) {
        return false;
    }
    const float localX = px - viewport.x;
    const float localY = py - viewport.y;
    const float ndcX = (2.0F * localX / viewport.width) - 1.0F;
    const float ndcY = (2.0F * localY / viewport.height) - 1.0F;
    const Vector4 p0 = invViewProj * Vector4(ndcX, ndcY, 0.0F, 1.0F);
    const Vector4 p1 = invViewProj * Vector4(ndcX, ndcY, 1.0F, 1.0F);
    if (std::fabs(p0.w) < 1.0e-6F || std::fabs(p1.w) < 1.0e-6F) {
        return false;
    }
    const Vector3 origin = (p0 * (1.0F / p0.w)).ToVector3();
    Vector3 dir = (p1 * (1.0F / p1.w)).ToVector3() - origin;
    if (dir.LengthSquared() < 1.0e-12F) {
        return false;
    }
    dir = dir.Normalized();
    outOrigin = origin;
    outDir = dir;
    return true;
}

}  // namespace

void EditorViewport::ResetCamera() noexcept {
    cameraController.ResetToDefault();
    transformGizmo.EndDrag();
    cameraController.EndOrbitDrag();
    dragTarget = nullptr;
    rmbDragDistSq = 0.0F;
    transformGizmo.SetMode(TransformGizmoMode::Translate);
}

bool EditorViewport::IsEditorChromeObject(const GameObject* object) noexcept {
    if (object == nullptr) {
        return true;
    }
    const Utf8String& name = object->GetName();
    return name == Utf8String("EditorGui") || name == Utf8String("EditorStatusHud");
}

bool EditorViewport::IsNonPickableObject(const GameObject* object) noexcept {
    if (IsEditorChromeObject(object)) {
        return true;
    }
    const Utf8String& name = object->GetName();
    return name == Utf8String("Ground");
}

GameObject* EditorViewport::ResolvePickTarget(GameObject* const hit) const noexcept {
    if (hit == nullptr) {
        return nullptr;
    }
    for (const GameObject* walk = hit; walk != nullptr; walk = walk->GetParent()) {
        if (IsNonPickableObject(walk)) {
            return nullptr;
        }
    }
    if (contentModel != nullptr) {
        if (GameObject* placedOwner = contentModel->FindPlacedOwner(hit)) {
            return placedOwner;
        }
    }
    return nullptr;
}

bool EditorViewport::IsPointerInViewport(
        const float cursorX,
        const float cursorY,
        const Ui::Rect& viewportRect) const noexcept {
    return viewportRect.Contains(cursorX, cursorY);
}

bool EditorViewport::TryBuildWorldRay(
        const Ui::Rect& viewportRect,
        const float cursorX,
        const float cursorY,
        const Matrix4& viewProj,
        Vector3& outOrigin,
        Vector3& outDir) const noexcept {
    Matrix4 invVp{};
    if (!viewProj.TryInvert(invVp)) {
        return false;
    }
    if (!ScreenToWorldRay(viewportRect, cursorX, cursorY, invVp, outOrigin, outDir)) {
        return false;
    }
    return true;
}

bool EditorViewport::RayIntersectPlaneY(
        const Vector3& ro,
        const Vector3& rd,
        const float planeY,
        Vector3& outHit) noexcept {
    if (std::fabs(rd.y) < 1.0e-6F) {
        return false;
    }
    const float t = (planeY - ro.y) / rd.y;
    if (t < 0.0F || t > 800.0F) {
        return false;
    }
    outHit = {ro.x + rd.x * t, ro.y + rd.y * t, ro.z + rd.z * t};
    return true;
}

bool EditorViewport::TryPickScene(
        Scene& scene,
        const Vector3& rayOrigin,
        const Vector3& rayDir,
        GameObject*& outObject,
        Vector3& outHit) noexcept {
    dragTarget = nullptr;
    SceneRay ray{};
    ray.origin = rayOrigin;
    ray.direction = rayDir;
    ray.tMin = 1.0e-4F;
    ray.tMax = 1.0e30F;
    SceneRaycastHit hit{};
    SceneRaycastOptions opts{};
    opts.pickSkinnedMeshes = true;
    if (!scene.RaycastPick(ray, hit, opts) || hit.object == nullptr) {
        return false;
    }
    if (IsEditorChromeObject(hit.object)) {
        return false;
    }
    GameObject* resolved = ResolvePickTarget(hit.object);
    if (resolved == nullptr) {
        return false;
    }
    dragTarget = resolved;
    dragPlaneY = hit.pointWorld.y;
    outObject = resolved;
    outHit = hit.pointWorld;
    return true;
}

float EditorViewport::SelectionGizmoExtent(GameObject* object) const noexcept {
    if (object == nullptr) {
        return 1.0F;
    }
    const TransformComponent* transform = object->GetComponent<TransformComponent>();
    if (transform == nullptr) {
        return 1.0F;
    }
    const Vector3 scale = transform->GetLocalTransform().scale;
    return std::max({scale.x, scale.y, scale.z, 1.0F});
}

void EditorViewport::BeginTransformRecording(GameObject& target) noexcept {
    transformRecordTarget = &target;
    transformRecording = true;
    if (TransformComponent* transform = target.GetComponent<TransformComponent>()) {
        transformRecordBefore = transform->GetLocalTransform();
    }
}

void EditorViewport::CommitTransformRecording() noexcept {
    if (!transformRecording || transformRecordTarget == nullptr || commandStack == nullptr) {
        CancelTransformRecording();
        return;
    }
    TransformComponent* transform = transformRecordTarget->GetComponent<TransformComponent>();
    if (transform == nullptr) {
        CancelTransformRecording();
        return;
    }
    const Transform after = transform->GetLocalTransform();
    if (!SetTransformCommand::NearlyEqual(transformRecordBefore, after)) {
        UniquePtr<SetTransformCommand> command =
                MakeUnique<SetTransformCommand>(*transformRecordTarget, transformRecordBefore, after);
        commandStack->Record(UniquePtr<IEditorCommand>(command.Release()));
    }
    CancelTransformRecording();
}

void EditorViewport::CancelTransformRecording() noexcept {
    transformRecordTarget = nullptr;
    transformRecording = false;
}

void EditorViewport::Simulate(
        const FrameTiming& timing,
        Scene& scene,
        IEngineContext& context,
        EditorSelection& selection,
        const Ui::Rect& viewportRect,
        const int framebufferWidth,
        const int framebufferHeight,
        Utf8String& statusLine,
        const bool editingEnabled) noexcept {
    IInput& input = context.GetInput();

    if (input.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        input.SetCursorCaptured(!input.IsCursorCaptured());
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_W) && editingEnabled) {
        transformGizmo.SetMode(TransformGizmoMode::Translate);
        statusLine = Utf8String("Gizmo: Move (W/E/R).");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_E) && editingEnabled) {
        transformGizmo.SetMode(TransformGizmoMode::Rotate);
        statusLine = Utf8String("Gizmo: Rotate (W/E/R).");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_R) && editingEnabled) {
        transformGizmo.SetMode(TransformGizmoMode::Scale);
        statusLine = Utf8String("Gizmo: Scale (W/E/R).");
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_Q) && editingEnabled) {
        transformGizmo.CycleMode();
        statusLine = Utf8String(transformGizmo.GetInteractionHint());
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_F)) {
        GameObject* selected = selection.GetPrimary();
        if (selected == nullptr) {
            statusLine = Utf8String("Select an object in the viewport first.");
        } else if (TransformComponent* transform = selected->GetComponent<TransformComponent>()) {
            cameraController.FocusOn(transform->GetLocalTransform().translation);
            statusLine = Utf8String("Camera focused on selection.");
        }
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_HOME)) {
        cameraController.ResetToDefault();
        statusLine = Utf8String("Camera reset to default view.");
    }

    float cursorX = 0.0F;
    float cursorY = 0.0F;
    input.GetCursorFramebufferPixels(cursorX, cursorY, framebufferWidth, framebufferHeight);
    const bool inViewport = IsPointerInViewport(cursorX, cursorY, viewportRect);

    if (input.IsCursorCaptured()) {
        cameraController.UpdateFlyNavigation(input, timing);
    } else {
        if (input.IsMouseButtonPressedThisFrame(1) && inViewport) {
            rmbDragDistSq = 0.0F;
        }
        if (input.IsMouseButtonDown(1) && inViewport && timing.frameIndex > 0) {
            const float mdx = input.GetMouseDeltaX();
            const float mdy = input.GetMouseDeltaY();
            rmbDragDistSq += mdx * mdx + mdy * mdy;
        }
        cameraController.UpdateEditorNavigation(input, timing, inViewport, UiConsumesGamePointer());
        if (input.IsMouseButtonReleasedThisFrame(0)) {
            cameraController.EndOrbitDrag();
        }
    }

    GameObject* selected = selection.GetPrimary();
    if (selected != nullptr) {
        selectionPulseTime += timing.deltaTimeSeconds;
    } else {
        selectionPulseTime = 0.0F;
    }

    if (input.IsCursorCaptured() || UiConsumesGamePointer()) {
        return;
    }

    const float aspect = (viewportRect.width > 1.0F && viewportRect.height > 1.0F)
            ? viewportRect.width / viewportRect.height
            : ((framebufferHeight > 0) ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight)
                                       : 1.0F);
    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.1F, 500.0F);
    const Matrix4 view = cameraController.camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    Vector3 rayOrigin{};
    Vector3 rayDir{};
    Vector3 groundHit{};
    const bool haveRay = TryBuildWorldRay(viewportRect, cursorX, cursorY, viewProj, rayOrigin, rayDir);
    const bool haveGround = haveRay && RayIntersectPlaneY(rayOrigin, rayDir, 0.0F, groundHit);
    if (haveGround) {
        lastGroundHit = groundHit;
    }

    if (input.IsMouseButtonReleasedThisFrame(0)) {
        if (transformRecording) {
            CommitTransformRecording();
        }
        dragTarget = nullptr;
        transformGizmo.EndDrag();
    }

    const bool altHeld = input.IsKeyDown(GLFW_KEY_LEFT_ALT) || input.IsKeyDown(GLFW_KEY_RIGHT_ALT);

    if (!editingEnabled) {
        return;
    }

    if (cameraController.IsOrbitDragging() && input.IsMouseButtonDown(0) && timing.frameIndex > 0) {
        cameraController.UpdateOrbitDrag(input);
    } else if (haveRay && input.IsMouseButtonPressedThisFrame(0) && inViewport) {
        bool handledPress = false;
        if (altHeld) {
            Vector3 pivot = cameraController.orbitPivot;
            if (selected != nullptr) {
                if (TransformComponent* selTr = selected->GetComponent<TransformComponent>()) {
                    pivot = selTr->GetLocalTransform().translation;
                }
            }
            cameraController.BeginOrbitDrag(pivot);
            handledPress = true;
        }
        if (!handledPress && selected != nullptr) {
            if (TransformComponent* selTr = selected->GetComponent<TransformComponent>()) {
                const float extent = SelectionGizmoExtent(selected);
                if (transformGizmo.TryBeginDrag(rayOrigin, rayDir, *selTr, extent, *selected)) {
                    dragTarget = selected;
                    BeginTransformRecording(*selected);
                    handledPress = true;
                    statusLine = Utf8String(transformGizmo.GetInteractionHint());
                }
            }
        }
        if (!handledPress) {
            GameObject* picked = nullptr;
            Vector3 pickHit{};
            if (TryPickScene(scene, rayOrigin, rayDir, picked, pickHit)) {
                selection.SetPrimary(picked);
                if (picked != nullptr) {
                    statusLine = picked->GetName();
                }
            } else if (haveGround) {
                selection.Clear();
                dragTarget = nullptr;
                statusLine = Utf8String("Selection cleared.");
            }
        }
    }

    if (!cameraController.IsOrbitDragging() && haveRay && input.IsMouseButtonDown(0) && dragTarget != nullptr &&
        transformGizmo.IsDragging()) {
        if (TransformComponent* transform = dragTarget->GetComponent<TransformComponent>()) {
            (void)transformGizmo.UpdateDrag(rayOrigin, rayDir, *transform);
        }
    } else if (!cameraController.IsOrbitDragging() && haveRay && input.IsMouseButtonDown(0) &&
               dragTarget != nullptr && !transformGizmo.IsDragging() &&
               transformGizmo.GetMode() == TransformGizmoMode::Translate) {
        if (!transformRecording) {
            BeginTransformRecording(*dragTarget);
        }
        Vector3 dragHit{};
        if (RayIntersectPlaneY(rayOrigin, rayDir, dragPlaneY, dragHit)) {
            if (TransformComponent* transform = dragTarget->GetComponent<TransformComponent>()) {
                const Vector3 translation = transform->GetLocalTransform().translation;
                transform->SetTranslation({dragHit.x, translation.y, dragHit.z});
            }
        }
    }

    (void)rmbDragDistSq;
}

void EditorViewport::AppendGizmoDraws(SceneRenderParams& params, EditorSelection& selection) const {
    GameObject* selected = selection.GetPrimary();
    if (selected == nullptr) {
        return;
    }
    TransformComponent* transform = selected->GetComponent<TransformComponent>();
    if (transform == nullptr) {
        return;
    }
    const float extent = SelectionGizmoExtent(selected);
    Array<SceneDrawItem> gizmoDraws;
    gizmoDraws.Reserve(12);
    transformGizmo.AppendDraws(*transform, extent, gizmoDraws);
    for (std::size_t i = 0; i < gizmoDraws.GetSize(); ++i) {
        params.draws.PushBack(gizmoDraws[i]);
    }
}

}  // namespace Spark::Editor
