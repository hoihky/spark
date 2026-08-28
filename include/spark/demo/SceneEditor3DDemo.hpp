#pragma once

#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/core/SceneInstanceTracker.hpp"
#include "spark/scene/core/SceneLoadSession.hpp"
#include "spark/scene/core/SceneManager.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"
#include "spark/scene/editor/SceneEditorPlaySession.hpp"
#include "spark/scene/editor/GltfImportService.hpp"
#include "spark/scene/editor/ScenePlacementActions.hpp"
#include "spark/scene/prefab/PrefabCatalog.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

#include <cstdio>
#include <cstring>
#include <string>

namespace Spark {

/**
 * 3D scene editor demo wired through reusable scene-editor services:
 * content model, placement command registry, load session, and play-mode state.
 */
class SceneEditor3DDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Unload(Spark::GameWorld& w);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);
    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    void SetStatusMessage(const Spark::Utf8String& msg);
    void ClearPlaced(Spark::GameWorld& w);
    void ClearUserLights(Spark::GameWorld& w);
    void UnloadEditorSceneContent(Spark::GameWorld& w);

    [[nodiscard]] bool IsUserLight(Spark::GameObject* go) const noexcept;
    void ValidateLightEditTarget() noexcept;
    static void SyncLightGizmoEmissive(Spark::GameObject* go) noexcept;

    [[nodiscard]] Spark::GameObject* AddUserPointLightAt(
            Spark::GameWorld& w,
            const Spark::Vector3& pos,
            const Spark::Vector3& color,
            float intensity,
            float range);

    [[nodiscard]] bool TryPickEditorRay(
            Spark::Scene& scene,
            const Spark::Vector3& rayOrigin,
            const Spark::Vector3& rayDir,
            bool pickMeshes,
            bool pickLights,
            Spark::Vector3& outHit) noexcept;

    void SaveSceneToFile(Spark::GameWorld& w);
    void LoadSceneFromFile(Spark::GameWorld& w);
    void FinalizeAsyncSceneLoad(Spark::GameWorld& w);
    void ReloadSceneForPlayMode(Spark::GameWorld& w);

    static void MaterialFilePath(char* out, std::size_t outSz) noexcept;
    void TrySaveSelectedMaterial(Spark::GameWorld& w);
    void TryLoadSelectedMaterial(Spark::GameWorld& w);

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
    [[nodiscard]] ScenePlacementContext MakePlacementContext(
            Spark::GameWorld& world,
            const Spark::Vector3& groundHit,
            Spark::GameObject* selection) noexcept;

    static void SetStatusFromPlacement(const char* message, void* userData) noexcept;
    static bool PlacementDeleteSelected(Spark::ScenePlacementContext& ctx);
    static bool PlacementSaveScene(Spark::ScenePlacementContext& ctx);
    static bool PlacementLoadScene(Spark::ScenePlacementContext& ctx);
    static bool PlacementDeleteAvailable(const Spark::ScenePlacementContext& ctx);
    static bool PlacementImportGltf(Spark::ScenePlacementContext& ctx);
    static void ReloadSceneForPlayModeStatic(Spark::GameWorld& world, void* userData) noexcept;

    SceneEditorContentModel contentModel{};
    SceneInstanceTracker instanceTracker{};
    PrefabCatalog prefabCatalog{};
    ScenePlacementActionRegistry placementActions{};
    SceneEditorPlaySession playSession{};
    GltfImportService gltfImportService{};
    Spark::FlyCamera camera{};
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset;
    Spark::SharedPtr<Spark::Mesh> groundAsset;
    Spark::GameObject* lightEditTarget = nullptr;
    DemoHelpHud helpHud{};
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
    Spark::Vector3 lastGroundHit{};
    float selectionPulseTime = 0.0F;
    Spark::Utf8String statusMessage{};
    Spark::UniquePtr<Spark::SceneManager> sceneManager;
    Spark::UniquePtr<Spark::SceneLoadSession> loadSession;
    Spark::SceneInstanceId loadedSceneId = Spark::kInvalidSceneInstanceId;
    Spark::SceneDocument pendingLoadDocument{};
    bool sceneLoadInProgress = false;
};

}  // namespace Spark
