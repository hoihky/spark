#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/MaterialComponent.hpp"
#include "spark/ecs/components/MeshComponent.hpp"
#include "spark/ecs/components/PointLightComponent.hpp"
#include "spark/ecs/components/TextOverlayComponent.hpp"
#include "spark/ecs/components/TransformComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/Scene.hpp"
#include "spark/scene/SceneInstanceId.hpp"
#include "spark/scene/SceneManager.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <string>

namespace Spark {

/**
 * 3D scene editor: right-click the viewport for mesh/light placement; LMB to select and move;
 * save/load scene text (v4 ECS snapshot; v3 and legacy v1/v2 still load) under the build runtime assets directory.
 */
class SceneEditor3DDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    enum class SceneEditorMenuAction : int {
        MeshDamagedHelmet = 0,
        MeshSheenChair,
        MeshUnitCube,
        LightWarm,
        LightCool,
        LightMagenta,
        DeleteSelected,
        SaveScene,
        LoadScene,
    };

    static void SceneFilePath(char* out, std::size_t outSz) noexcept;


    void SetStatusMessage(const Spark::Utf8String& msg);


    void ClearPlaced(Spark::GameWorld& w);


    void ClearUserLights(Spark::GameWorld& w);


    [[nodiscard]] bool IsUserLight(Spark::GameObject* go) const noexcept;


    void ValidateLightEditTarget() noexcept;


    static void SyncLightGizmoEmissive(Spark::GameObject* go) noexcept;


    static void LightPresetParams(int preset, Spark::Vector3& outColor, float& outIntensity, float& outRange) noexcept;


    [[nodiscard]] Spark::GameObject* AddUserPointLightAt(
            Spark::GameWorld& w,
            const Spark::Vector3& pos,
            const Spark::Vector3& color,
            float intensity,
            float range);


    [[nodiscard]] bool TrySpawnUserPointLight(
            Spark::GameWorld& w, const Spark::Vector3& groundHit, int presetIndex) noexcept;


    /**
     * Screen-space pick: ray vs mesh triangles (world space) and light spheres.
     * Sets <c>dragPlaced</c>, <c>dragPlaneY</c>, and optional <c>outHit</c>.
     */
    [[nodiscard]] bool TryPickEditorRay(
            Spark::Scene& scene,
            const Spark::Vector3& rayOrigin,
            const Spark::Vector3& rayDir,
            bool pickMeshes,
            bool pickLights,
            Spark::Vector3& outHit) noexcept;


    [[nodiscard]] static const char* PresetRelPath(int idx) noexcept;


    [[nodiscard]] bool TryPlaceAtPreset(Spark::GameWorld& w, const Spark::Vector3& hitXZ, int presetIndex);


    void SaveSceneToFile(Spark::GameWorld& /*w*/);


    void LoadSceneFromFile(Spark::GameWorld& w);

    void FinalizeAsyncSceneLoad(Spark::GameWorld& w);

    static void SortObjectsById(Spark::Array<Spark::GameObject*>& objects) noexcept;


    void SetupContextMenuCanvas(Spark::GameWorld& w);


    void OpenSceneContextMenu(
            float menuX,
            float menuY,
            const Spark::Vector3& groundHit,
            Spark::GameObject* selection,
            Spark::GameWorld& world);


    void RemoveEditorSelection(Spark::GameWorld& w, Spark::GameObject* go) noexcept;


    void FocusCameraOnSelection() noexcept;


    void ResetEditorCamera() noexcept;


    [[nodiscard]] bool IsPointerInEditorViewport(float cursorX, int framebufferWidth) const noexcept;


    [[nodiscard]] float SelectionGizmoExtent(Spark::GameObject* go) const noexcept;


    Spark::Array<Spark::GameObject*> roots{};
    Spark::Array<Spark::GameObject*> placed{};
    Spark::Array<Spark::Utf8String> placedRel{};
    Spark::FlyCamera camera{};
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset;
    Spark::SharedPtr<Spark::Mesh> groundAsset;
    Spark::GameObject* guiCanvasObject = nullptr;
    Spark::GuiCanvasComponent* guiCanvas = nullptr;
    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    Spark::GameObject* lightEditTarget = nullptr;
    Spark::Array<Spark::GameObject*> userLights{};
    float fpsSmoothed = 0.0F;
    Spark::GameObject* selectedObject = nullptr;
    Spark::GameObject* dragPlaced = nullptr;
    float dragPlaneY = 0.0F;
    Spark::Vector3 cameraOrbitPivot{};
    float cameraOrbitDistance = 18.0F;
    bool orbitDragActive = false;
    float rmbDragDistSq = 0.0F;
    int gizmoDragAxis = -1;
    float gizmoDragStartLineS = 0.0F;
    Spark::Vector3 gizmoDragStartTranslation{};
    float selectionPulseTime = 0.0F;
    Spark::Utf8String statusMessage{};
    Spark::UniquePtr<Spark::SceneManager> sceneManager;
    Spark::SceneInstanceId loadedSceneId = Spark::kInvalidSceneInstanceId;
    Spark::SceneDocument pendingLoadDocument{};
    bool sceneLoadInProgress = false;
};

}  // namespace Spark
