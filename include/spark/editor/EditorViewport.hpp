#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorSelection.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Transform.hpp"
#include "spark/scene/camera/FlyCamera.hpp"
#include "spark/scene/editor/SceneEditorCameraController.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"
#include "spark/scene/editor/TransformGizmo.hpp"
#include "spark/ui/core/UiTypes.hpp"

namespace Spark {

class GameObject;
class IEngineContext;
class IInput;
class Scene;
struct FrameTiming;
struct SceneRenderParams;

namespace Editor {

class EditorCommandStack;

/**
 * Center-pane 3D viewport: camera navigation, ray pick, and transform gizmo.
 * Consumes dock center bounds for input gating and render scissor alignment.
 */
class EditorViewport final {
public:
    void ResetCamera() noexcept;
    void SetCommandStack(EditorCommandStack* stack) noexcept { commandStack = stack; }
    void SetContentModel(SceneEditorContentModel* model) noexcept { contentModel = model; }

    void Simulate(
            const FrameTiming& timing,
            Scene& scene,
            IEngineContext& context,
            EditorSelection& selection,
            const Ui::Rect& viewportRect,
            int framebufferWidth,
            int framebufferHeight,
            Utf8String& statusLine,
            bool editingEnabled) noexcept;

    void AppendGizmoDraws(SceneRenderParams& params, EditorSelection& selection) const;

    [[nodiscard]] SceneEditorCameraController& GetCameraController() noexcept { return cameraController; }
    [[nodiscard]] const FlyCamera& GetCamera() const noexcept { return cameraController.camera; }
    [[nodiscard]] FlyCamera& GetCamera() noexcept { return cameraController.camera; }
    [[nodiscard]] const Vector3& GetLastGroundHit() const noexcept { return lastGroundHit; }
    [[nodiscard]] float GetSelectionPulseTime() const noexcept { return selectionPulseTime; }
    [[nodiscard]] bool IsTransformEditActive() const noexcept { return transformRecording; }
    [[nodiscard]] GameObject* GetTransformEditTarget() const noexcept { return transformRecordTarget; }

    void SetGizmoMode(TransformGizmoMode mode) noexcept { transformGizmo.SetMode(mode); }
    [[nodiscard]] TransformGizmoMode GetGizmoMode() const noexcept { return transformGizmo.GetMode(); }

private:
    void BeginTransformRecording(GameObject& target) noexcept;
    void CommitTransformRecording() noexcept;
    void CancelTransformRecording() noexcept;

    [[nodiscard]] bool IsPointerInViewport(float cursorX, float cursorY, const Ui::Rect& viewportRect) const noexcept;
    [[nodiscard]] bool TryBuildWorldRay(
            const Ui::Rect& viewportRect,
            float cursorX,
            float cursorY,
            const Matrix4& viewProj,
            Vector3& outOrigin,
            Vector3& outDir) const noexcept;

    [[nodiscard]] bool TryPickScene(
            Scene& scene,
            const Vector3& rayOrigin,
            const Vector3& rayDir,
            GameObject*& outObject,
            Vector3& outHit) noexcept;

    [[nodiscard]] static bool RayIntersectPlaneY(
            const Vector3& ro,
            const Vector3& rd,
            float planeY,
            Vector3& outHit) noexcept;

    [[nodiscard]] static bool IsEditorChromeObject(const GameObject* object) noexcept;
    [[nodiscard]] static bool IsNonPickableObject(const GameObject* object) noexcept;
    [[nodiscard]] GameObject* ResolvePickTarget(GameObject* hit) const noexcept;
    [[nodiscard]] float SelectionGizmoExtent(GameObject* object) const noexcept;

    SceneEditorCameraController cameraController{};
    TransformGizmo transformGizmo{};
    EditorCommandStack* commandStack = nullptr;
    SceneEditorContentModel* contentModel = nullptr;
    GameObject* dragTarget = nullptr;
    GameObject* transformRecordTarget = nullptr;
    Transform transformRecordBefore{};
    float dragPlaneY = 0.0F;
    float rmbDragDistSq = 0.0F;
    float selectionPulseTime = 0.0F;
    bool transformRecording = false;
    Vector3 lastGroundHit{Vector3::Zero};
};

}  // namespace Editor
}  // namespace Spark
