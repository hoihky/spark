#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/editor/EditorCommandStack.hpp"
#include "spark/editor/EditorContext.hpp"
#include "spark/editor/EditorTextureCatalog.hpp"
#include "spark/editor/EditorSelection.hpp"
#include "spark/editor/EditorTypes.hpp"
#include "spark/editor/EditorViewport.hpp"
#include "spark/editor/panels/EditorDockShell.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/core/SceneInstanceId.hpp"
#include "spark/scene/core/SceneInstanceTracker.hpp"
#include "spark/scene/core/SceneLoadSession.hpp"
#include "spark/scene/core/SceneManager.hpp"
#include "spark/scene/editor/GltfImportService.hpp"
#include "spark/scene/editor/SceneEditorAssetCatalog.hpp"
#include "spark/scene/editor/SceneEditorAssetBrowser.hpp"
#include "spark/scene/editor/SceneEditorContentModel.hpp"
#include "spark/scene/editor/SceneEditorPlaySession.hpp"
#include "spark/scene/editor/ScenePlacementActions.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/prefab/PrefabCatalog.hpp"
#include "spark/scene/serialization/SceneDocument.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class IEngineContext;
struct FrameTiming;

namespace Editor {

enum class PendingFileDialog : std::uint8_t {
    None = 0,
    NewProject,
    OpenProject,
    OpenScene,
    SaveProject,
    SaveProjectAs,
    SaveScene,
    SaveSceneAs,
};

class HierarchyPanel;
class InspectorPanel;
class ProjectBrowserPanel;

/**
 * Central editor coordinator: scene bootstrap, UI shell, viewport service, panel tick.
 * Used by spark_editor/EditorGame (implements IGame).
 */
class EditorApplication final : private ISceneEditorAssetBrowserHost {
public:
    EditorApplication();
    ~EditorApplication();
    void OnAttach(Scene& scene, IEngineContext& context);
    void OnDetach(Scene& scene);
    void OnUpdate(const FrameTiming& timing, Scene& scene, IEngineContext& context);
    void OnRender(Scene& scene, IEngineContext& context);

    [[nodiscard]] EditorMode GetMode() const noexcept { return mode; }
    [[nodiscard]] const Utf8String& GetStatusLine() const noexcept { return statusLine; }

private:
    void BootstrapDefaultScene(GameWorld& world);
    void BuildEditorUi(GameWorld& world);
    void TickPanels(const FrameTiming& timing, Scene& scene, IEngineContext& engineContext);
    void HighlightSelection(Scene& scene);
    void PumpSceneLoads(GameWorld& world);
    void SaveSceneToFile(GameWorld& world, bool forcePicker = false);
    void LoadSceneFromPath(GameWorld& world, const char* relativeScenePath);
    void ReloadSceneForPlayMode(GameWorld& world);
    void FinalizeAsyncSceneLoad(GameWorld& world);
    void UnloadEditorSceneContent(GameWorld& world);
    void EnsureEditorBootstrapScene(GameWorld& world);
    void PlacePrefabFromAsset(const SceneEditorAssetEntry& entry);
    void HandlePlayModeInput(IEngineContext& context, GameWorld& world);
    void HandleEditModeInput(IEngineContext& context, GameWorld& world);
    void OnSelectionChanged();
    void MarkSceneDirty() noexcept;
    void ClearSceneDirty() noexcept;
    void UpdateStatusHud() noexcept;
    void SaveEditorLayout() noexcept;
    void ApplyOpenProject(GameWorld& world);
    void NewProject();
    void OpenProject();
    void SaveProject();
    void SaveProjectAs();
    void OpenScene();
    void SaveScene(bool forcePicker);
    void ProcessPendingFileDialogs();
    void PaintFileMenuBar();
    [[nodiscard]] Utf8String GetEditorAssetsRoot() const noexcept;
    [[nodiscard]] Utf8String JoinProjectAssetPath(const char* relativePath) const noexcept;
    [[nodiscard]] Utf8String ResolveEditorScenePath(const char* relativeOrAbsolutePath) const noexcept;
    [[nodiscard]] bool ShouldCaptureSceneEntity(const GameObject* object) const noexcept;
    [[nodiscard]] static bool EnsureParentDirectoryExists(const char* filePath) noexcept;
    [[nodiscard]] ScenePlacementContext MakePlacementContext(
            GameWorld& world,
            GameObject* selected) noexcept;

    static void OnSelectionChangedStatic(void* userData) noexcept;
    static void OnCommandStackChangedStatic(void* userData) noexcept;
    static void ReloadSceneForPlayModeStatic(GameWorld& world, void* userData) noexcept;
    static void SetStatusFromPlaySession(const char* message, void* userData) noexcept;

    void OnAssetBrowserPlacePrefab(const SceneEditorAssetEntry& entry) override;
    void OnAssetBrowserLoadScene(const SceneEditorAssetEntry& entry) override;
    void OnAssetBrowserImportGltf() override;
    void OnAssetBrowserRefreshCatalog() override;

    EditorMode mode = EditorMode::Edit;
    WorkspaceDimension workspace = WorkspaceDimension::ThreeD;
    EditorSelection selection{};
    EditorCommandStack commandStack{};
    EditorProject project{};
    EditorTextureCatalog textureCatalog{};
    EditorContext context{};
    EditorDockShell dock{};
    EditorViewport viewport{};

    UniquePtr<HierarchyPanel> hierarchyPanel;
    UniquePtr<InspectorPanel> inspectorPanel;
    UniquePtr<ProjectBrowserPanel> projectPanel;

    SceneEditorContentModel contentModel{};
    SceneEditorAssetCatalog assetCatalog{};
    PrefabCatalog prefabCatalog{};
    ScenePlacementActionRegistry placementActions{};
    GltfImportService gltfImportService{};
    SceneEditorPlaySession playSession{};

    UniquePtr<SceneManager> sceneManager;
    UniquePtr<SceneLoadSession> loadSession;
    SceneInstanceTracker instanceTracker{};
    SceneInstanceId loadedSceneId = kInvalidSceneInstanceId;
    SceneDocument pendingLoadDocument{};
    bool sceneLoadInProgress = false;

    GameObject* guiCanvasObject = nullptr;
    UiCanvasComponent* guiCanvas = nullptr;
    GameObject* fpsHudObject = nullptr;
    class TextOverlayComponent* fpsText = nullptr;
    GameObject* bootstrapGround = nullptr;
    GameObject* bootstrapSun = nullptr;
    GameObject* highlightedObject = nullptr;

    SharedPtr<Mesh> groundMesh;
    SharedPtr<Mesh> unitCubeAsset;
    Utf8String statusLine{"Spark Editor — Ctrl+S save · Ctrl+Z undo · P play · Esc stop"};
    bool sceneDirty = false;
    bool uiBuilt = false;
    bool projectLocationUserSet = false;
    PendingFileDialog pendingFileDialog = PendingFileDialog::None;
    Utf8String activeSceneRelativePath{};
    Utf8String activeSceneAbsolutePath{};
};

}  // namespace Editor
}  // namespace Spark
