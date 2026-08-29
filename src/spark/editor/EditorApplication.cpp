#include "spark/editor/EditorApplication.hpp"

#include "spark/config.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/editor/EditorDefaultLayout.hpp"
#include "spark/editor/EditorUiFont.hpp"
#include "spark/editor/panels/HierarchyPanel.hpp"
#include "spark/editor/panels/InspectorPanel.hpp"
#include "spark/editor/panels/ProjectBrowserPanel.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/ui/runtime/EditorLayoutStore.hpp"
#include "spark/ui/runtime/UiScene.hpp"
#include "spark/ui/core/UiTheme.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/render/platform/Window.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/core/ScenePartitionKind.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/prefab/PrefabInstantiator.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"

#include <GLFW/glfw3.h>

#include <cstring>
#include <format>

namespace Spark::Editor {

EditorApplication::EditorApplication() = default;

EditorApplication::~EditorApplication() = default;

void EditorApplication::BootstrapDefaultScene(GameWorld& world) {
    groundMesh = MakeShared<Mesh>(Utf8String("EditorGround"));
    *groundMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent);
    world.RegisterMesh(groundMesh, "spark/editor/ground");

    unitCubeAsset = MakeShared<Mesh>(Utf8String("EditorUnitCube"));
    *unitCubeAsset = Mesh::CreateUnitCube();
    world.RegisterMesh(unitCubeAsset, "spark/scene_editor/unit_cube");

    GameObject* ground = world.CreateGameObject();
    ground->GetName() = Utf8String("Ground");
    ground->AddComponent<TransformComponent>();
    ground->AddComponent<MeshComponent>(
            groundMesh, SceneMeshSlot::GroundPlane, Vector3{0.45F, 0.48F, 0.5F});
    contentModel.TrackRoot(ground);

    GameObject* sun = world.CreateGameObject();
    sun->GetName() = Utf8String("Sun");
    TransformComponent* sunTr = sun->AddComponent<TransformComponent>();
    sunTr->SetTranslation({18.0F, 28.0F, 12.0F});
    sun->AddComponent<PointLightComponent>(Vector3{1.0F, 0.96F, 0.9F}, 2.4F, 120.0F);
    contentModel.TrackRoot(sun);

    GameObject* sample = world.CreateGameObject();
    sample->GetName() = Utf8String("SampleCube");
    TransformComponent* sampleTr = sample->AddComponent<TransformComponent>();
    sampleTr->SetTranslation({0.0F, 0.5F, 0.0F});
    sampleTr->SetUniformScale(1.0F);
    sample->AddComponent<MeshComponent>(SharedPtr<Mesh>{}, SceneMeshSlot::UnitCube, Vector3{0.72F, 0.35F, 0.28F});
    if (MaterialComponent* mat = sample->AddComponent<MaterialComponent>()) {
        mat->SetMetallic(0.15F);
        mat->SetRoughness(0.42F);
    }
    contentModel.TrackRoot(sample);
    contentModel.TrackPlaced(sample, Utf8String("builtin:unit_cube"));
}

void EditorApplication::BuildEditorUi(GameWorld& world) {
    if (uiBuilt) {
        return;
    }

    hierarchyPanel = MakeUnique<HierarchyPanel>();
    inspectorPanel = MakeUnique<InspectorPanel>();
    projectPanel = MakeUnique<ProjectBrowserPanel>();
    projectPanel->BindAssetCatalog(assetCatalog);

    selection.SetOnChanged(&EditorApplication::OnSelectionChangedStatic, this);
    viewport.SetCommandStack(&commandStack);
    viewport.SetContentModel(&contentModel);

    context.world = &world;
    context.selection = &selection;
    context.project = &project;
    context.commandStack = &commandStack;
    context.viewport = &viewport;
    context.textureCatalog = &textureCatalog;
    context.contentModel = &contentModel;
    context.assetCatalog = &assetCatalog;
    context.assetHost = this;
    context.mode = mode;
    context.workspace = workspace;
    context.statusLine = statusLine;

    for (IEditorPanel* panel :
            {static_cast<IEditorPanel*>(hierarchyPanel.Get()),
                    static_cast<IEditorPanel*>(inspectorPanel.Get()),
                    static_cast<IEditorPanel*>(projectPanel.Get())}) {
        panel->OnAttach(context);
    }

    Ui::SceneEditorLayoutSettings layout{};
    if (!Ui::TryLoadSceneEditorLayout(layout)) {
        EditorDefaultLayout::ApplyTo(layout);
    }
    dock.ApplyLayout(layout);
    dock.SetGizmoBinding(&viewport, &statusLine);

    dock.SetPanels(
            hierarchyPanel->ReleaseRootElement(),
            projectPanel->ReleaseRootElement(),
            inspectorPanel->ReleaseRootElement());

    guiCanvasObject = world.CreateGameObject();
    guiCanvasObject->GetName() = Utf8String("EditorGui");
    guiCanvas = guiCanvasObject->AddComponent<UiCanvasComponent>();
    guiCanvas->SetSortOrder(240);
    guiCanvas->SetTheme(Ui::UiTheme::SceneEditorDark());
    guiCanvas->SetRoot(dock.ReleaseRootElement());

    fpsHudObject = world.CreateGameObject();
    fpsHudObject->GetName() = Utf8String("EditorStatusHud");
    fpsText = fpsHudObject->AddComponent<TextOverlayComponent>();
    fpsText->SetScreenPosition(12.0F, 78.0F);
    fpsText->SetFontSizePixels(16.0F);
    fpsText->SetColor({0.9F, 0.93F, 0.98F});

    const Utf8String defaultProjectPath(SPARK_BUILD_ASSETS_DIR);
    (void)project.OpenExisting(defaultProjectPath.CStr());
    assetCatalog.Refresh();
    textureCatalog.Refresh();

    uiBuilt = true;
}

ScenePlacementContext EditorApplication::MakePlacementContext(
        GameWorld& world,
        GameObject* selected) noexcept {
    return ScenePlacementContext{
            world,
            *sceneManager,
            contentModel,
            instanceTracker,
            prefabCatalog,
            viewport.GetLastGroundHit(),
            selected,
            unitCubeAsset,
            nullptr,
            &EditorApplication::SetStatusFromPlaySession,
            this,
    };
}

void EditorApplication::OnSelectionChangedStatic(void* userData) noexcept {
    if (userData != nullptr) {
        static_cast<EditorApplication*>(userData)->OnSelectionChanged();
    }
}

void EditorApplication::OnSelectionChanged() {
    if (hierarchyPanel != nullptr) {
        hierarchyPanel->SyncToSelection();
    }
}

void EditorApplication::SetStatusFromPlaySession(const char* message, void* userData) noexcept {
    if (userData != nullptr && message != nullptr) {
        static_cast<EditorApplication*>(userData)->statusLine = Utf8String(message);
    }
}

void EditorApplication::ReloadSceneForPlayModeStatic(GameWorld& world, void* userData) noexcept {
    if (userData != nullptr) {
        static_cast<EditorApplication*>(userData)->ReloadSceneForPlayMode(world);
    }
}

void EditorApplication::HighlightSelection(Scene& /*scene*/) {
    GameObject* selected = selection.GetPrimary();
    if (highlightedObject != nullptr && highlightedObject != selected) {
        if (MaterialComponent* prev = highlightedObject->GetComponent<MaterialComponent>()) {
            prev->SetEmissive(Vector3{}, 0.0F);
        }
        highlightedObject = nullptr;
    }
    if (selected == nullptr) {
        return;
    }
    if (MaterialComponent* mat = selected->GetComponent<MaterialComponent>()) {
        mat->SetEmissive({0.35F, 0.28F, 0.12F}, 0.45F);
        highlightedObject = selected;
    }
}

void EditorApplication::FinalizeAsyncSceneLoad(GameWorld& world) {
    if (loadedSceneId == kInvalidSceneInstanceId) {
        return;
    }
    SceneEditorContentBindingHooks hooks{};
    hooks.unitCubeAsset = &unitCubeAsset;
    contentModel.IntegrateLoadedInstance(world, loadedSceneId, pendingLoadDocument, hooks);
    instanceTracker.Register(loadedSceneId);
    selection.Clear();
    pendingLoadDocument = SceneDocument{};
}

void EditorApplication::PumpSceneLoads(GameWorld& world) {
    if (!sceneManager) {
        return;
    }
    sceneManager->Pump();
    if (!sceneLoadInProgress || loadedSceneId == kInvalidSceneInstanceId) {
        return;
    }
    if (sceneManager->IsSceneReady(loadedSceneId)) {
        FinalizeAsyncSceneLoad(world);
        statusLine = Utf8String("Scene loaded.");
        sceneLoadInProgress = false;
    } else if (sceneManager->HasSceneFailed(loadedSceneId)) {
        sceneManager->UnloadScene(loadedSceneId);
        loadedSceneId = kInvalidSceneInstanceId;
        sceneLoadInProgress = false;
        pendingLoadDocument = SceneDocument{};
        statusLine = Utf8String("Scene load failed.");
    }
}

void EditorApplication::SaveSceneToFile(GameWorld& world) {
    const Utf8String path = ScenePathResolver::BuildRuntimePath("scenes", "editor_session.sparkscene");
    SceneCaptureContext captureCtx = contentModel.BuildCaptureContext();
    const auto includeEntity = [this](const GameObject* go) -> bool { return contentModel.ShouldCapture(go); };

    SceneSerializer serializer;
    SceneDocument document = serializer.Capture(world, captureCtx, includeEntity);
    document.header.name = Utf8String("EditorSession");
    document.header.assetsRoot = Utf8String(ScenePathResolver::AssetsRoot());
    if (!serializer.WriteToFile(document, path.CStr())) {
        statusLine = Utf8String("Save failed (could not open file).");
        return;
    }
    statusLine = Utf8String(
            std::format("Saved {} entities → scenes/editor_session.sparkscene", document.entities.GetSize()).c_str());
}

void EditorApplication::LoadSceneFromPath(GameWorld& world, const char* relativeScenePath) {
    if (relativeScenePath == nullptr || relativeScenePath[0] == '\0' || !sceneManager) {
        statusLine = Utf8String("Invalid scene path.");
        return;
    }
    const Utf8String path = ScenePathResolver::ResolveReadablePath(relativeScenePath);
    if (!ScenePathResolver::FileExists(path.CStr())) {
        statusLine = Utf8String("Scene file not found.");
        return;
    }
    SceneDocument document;
    SceneDeserializer deserializer;
    if (!deserializer.ReadFromFile(path.CStr(), document)) {
        statusLine = Utf8String("Invalid scene file.");
        return;
    }
    if (loadedSceneId != kInvalidSceneInstanceId) {
        instanceTracker.UnloadAll(*sceneManager);
        loadedSceneId = kInvalidSceneInstanceId;
    }
    pendingLoadDocument = document;
    SceneLoadOptions options{};
    options.assetsRoot = ScenePathResolver::AssetsRoot();
    options.additive = true;
    loadedSceneId = sceneManager->BeginLoadSceneAsync(document, path.CStr(), options);
    if (loadedSceneId == kInvalidSceneInstanceId) {
        pendingLoadDocument = SceneDocument{};
        statusLine = Utf8String("Scene load failed to start.");
        return;
    }
    sceneLoadInProgress = true;
    selection.Clear();
    if (sceneManager->IsSceneReady(loadedSceneId)) {
        FinalizeAsyncSceneLoad(world);
        statusLine = Utf8String("Scene loaded.");
        sceneLoadInProgress = false;
    } else {
        statusLine = Utf8String("Loading scene…");
    }
}

void EditorApplication::ReloadSceneForPlayMode(GameWorld& world) {
    LoadSceneFromPath(world, "scenes/editor_session.sparkscene");
    if (sceneLoadInProgress && loadSession) {
        while (sceneLoadInProgress) {
            loadSession->Pump();
            if (loadedSceneId != kInvalidSceneInstanceId && loadSession->IsReady(loadedSceneId)) {
                FinalizeAsyncSceneLoad(world);
                sceneLoadInProgress = false;
            } else if (loadedSceneId != kInvalidSceneInstanceId && loadSession->HasFailed(loadedSceneId)) {
                sceneLoadInProgress = false;
                statusLine = Utf8String("Play mode failed (scene load error).");
                return;
            }
        }
    }
}

void EditorApplication::PlacePrefabFromAsset(const SceneEditorAssetEntry& entry) {
    if (entry.kind != SceneEditorAssetKind::Prefab || !sceneManager) {
        statusLine = Utf8String("Select a prefab in the asset browser.");
        return;
    }
    const Utf8String path = ScenePathResolver::ResolveReadablePath(entry.relativePath.CStr());
    const Vector3 groundHit = viewport.GetLastGroundHit();
    PrefabInstantiateOptions options{};
    options.assetsRoot = ScenePathResolver::AssetsRoot();
    options.position = {groundHit.x, 0.0F, groundHit.z};
    options.additive = true;
    options.pumpUntilReady = true;

    const PrefabInstantiateResult result = sceneManager->InstantiatePrefab(path.CStr(), options);
    if (!result.ready || result.rootObjects.IsEmpty()) {
        statusLine = Utf8String("Could not instantiate prefab.");
        return;
    }
    instanceTracker.Register(result.instanceId);
    for (std::size_t i = 0; i < result.rootObjects.GetSize(); ++i) {
        GameObject* root = result.rootObjects[i];
        if (root == nullptr) {
            continue;
        }
        contentModel.TrackRoot(root);
        contentModel.TrackPlaced(root, entry.relativePath);
    }
    selection.SetPrimary(result.rootObjects[0]);
    statusLine = Utf8String("Placed prefab.");
}

void EditorApplication::OnAssetBrowserPlacePrefab(const SceneEditorAssetEntry& entry) {
    if (playSession.IsActive()) {
        return;
    }
    PlacePrefabFromAsset(entry);
}

void EditorApplication::OnAssetBrowserLoadScene(const SceneEditorAssetEntry& entry) {
    if (playSession.IsActive() || context.world == nullptr) {
        return;
    }
    if (entry.kind != SceneEditorAssetKind::Scene) {
        statusLine = Utf8String("Select a scene in the asset browser.");
        return;
    }
    LoadSceneFromPath(*context.world, entry.relativePath.CStr());
}

void EditorApplication::OnAssetBrowserImportGltf() {
    if (playSession.IsActive() || context.world == nullptr) {
        return;
    }
    ScenePlacementContext placementCtx =
            MakePlacementContext(*context.world, selection.GetPrimary());
    const GltfImportService::ImportResult importResult =
            gltfImportService.ImportFromFilePickerAndPlace(*context.world, placementCtx);
    statusLine = importResult.message;
    if (importResult.ok) {
        assetCatalog.Refresh();
        if (projectPanel != nullptr) {
            projectPanel->GetAssetBrowser().RefreshListFromCatalog();
        }
    }
}

void EditorApplication::OnAssetBrowserRefreshCatalog() {
    assetCatalog.Refresh();
    if (projectPanel != nullptr) {
        projectPanel->GetAssetBrowser().RefreshListFromCatalog();
    }
    statusLine = Utf8String("Asset list refreshed.");
}

void EditorApplication::HandleEditModeInput(IEngineContext& context) {
    IInput& input = context.GetInput();
    const bool ctrlDown = input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);

    if (ctrlDown && input.IsKeyPressedThisFrame(GLFW_KEY_Z)) {
        if (commandStack.TryUndo()) {
            statusLine = Utf8String("Undo.");
        }
    }
    if (ctrlDown && input.IsKeyPressedThisFrame(GLFW_KEY_Y)) {
        if (commandStack.TryRedo()) {
            statusLine = Utf8String("Redo.");
        }
    }
}

void EditorApplication::HandlePlayModeInput(IEngineContext& engineContext, GameWorld& world) {
    IInput& input = engineContext.GetInput();
    if (input.IsKeyPressedThisFrame(GLFW_KEY_P) && !playSession.IsActive()) {
        SaveSceneToFile(world);
        SceneEditorPlaySession::Dependencies deps{
                .loadSession = *loadSession,
                .content = contentModel,
                .instances = instanceTracker,
                .camera = viewport.GetCamera(),
                .orbitPivot = viewport.GetCameraController().orbitPivot,
                .orbitDistance = viewport.GetCameraController().orbitDistance,
                .reloadScene = &EditorApplication::ReloadSceneForPlayModeStatic,
                .setStatus = &EditorApplication::SetStatusFromPlaySession,
                .userData = this,
        };
        mode = EditorMode::Play;
        this->context.mode = mode;
        playSession.Enter(world, engineContext, deps);
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_ESCAPE) && playSession.IsActive()) {
        SceneEditorPlaySession::Dependencies deps{
                .loadSession = *loadSession,
                .content = contentModel,
                .instances = instanceTracker,
                .camera = viewport.GetCamera(),
                .orbitPivot = viewport.GetCameraController().orbitPivot,
                .orbitDistance = viewport.GetCameraController().orbitDistance,
                .reloadScene = &EditorApplication::ReloadSceneForPlayModeStatic,
                .setStatus = &EditorApplication::SetStatusFromPlaySession,
                .userData = this,
        };
        playSession.Exit(world, engineContext, deps);
        mode = EditorMode::Edit;
        this->context.mode = mode;
    }
}

void EditorApplication::TickPanels(const FrameTiming& timing, Scene& scene, IEngineContext& engineContext) {
    context.world = &scene.GetWorld();
    context.scene = &scene;
    context.engine = &engineContext;
    context.selection = &selection;
    context.commandStack = &commandStack;
    context.viewport = &viewport;
    context.textureCatalog = &textureCatalog;
    context.contentModel = &contentModel;
    context.mode = mode;
    context.statusLine = statusLine;
    if (hierarchyPanel) {
        hierarchyPanel->OnTick(timing, context);
    }
    if (inspectorPanel) {
        inspectorPanel->OnTick(timing, context);
    }
    if (projectPanel) {
        projectPanel->OnTick(timing, context);
    }
}

void EditorApplication::OnAttach(Scene& scene, IEngineContext& /*context*/) {
    scene.SetSpatialPartitionKind(ScenePartitionKind::BoundingVolumeHierarchy);
    MountEditorUiFonts(scene.GetWorld());
    contentModel.ClearLists();
    instanceTracker.Clear();
    prefabCatalog.Clear();
    placementActions.Clear();
    PrefabCatalog::RegisterDemoDefaults(prefabCatalog);
    ScenePlacementActionRegistry::RegisterDemoDefaults(placementActions);
    ScenePlacementActionRegistry::RegisterPrefabActions(placementActions, prefabCatalog);
    gltfImportService.Bind(&prefabCatalog, &placementActions);

    BootstrapDefaultScene(scene.GetWorld());
    sceneManager = MakeUnique<SceneManager>(scene.GetWorld());
    loadSession = MakeUnique<SceneLoadSession>(*sceneManager);
    BuildEditorUi(scene.GetWorld());
    viewport.ResetCamera();
}

void EditorApplication::OnDetach(Scene& scene) {
    GameWorld& world = scene.GetWorld();
    if (guiCanvasObject != nullptr) {
        world.DestroyGameObject(guiCanvasObject);
        guiCanvasObject = nullptr;
        guiCanvas = nullptr;
    }
    if (fpsHudObject != nullptr) {
        world.DestroyGameObject(fpsHudObject);
        fpsHudObject = nullptr;
        fpsText = nullptr;
    }
    if (hierarchyPanel) {
        hierarchyPanel->OnDetach();
    }
    if (inspectorPanel) {
        inspectorPanel->OnDetach();
    }
    if (projectPanel) {
        projectPanel->OnDetach();
    }
    hierarchyPanel.Reset();
    inspectorPanel.Reset();
    projectPanel.Reset();
    if (sceneManager) {
        instanceTracker.UnloadAll(*sceneManager);
    }
    instanceTracker.Clear();
    contentModel.ClearLists();
    loadSession.Reset();
    sceneManager.Reset();
    commandStack.Clear();
    uiBuilt = false;
}

void EditorApplication::OnUpdate(const FrameTiming& timing, Scene& scene, IEngineContext& engineContext) {
    int fbW = 0;
    int fbH = 0;
    engineContext.GetFramebufferSize(fbW, fbH);
    float contentScaleX = 1.0F;
    float contentScaleY = 1.0F;
    engineContext.GetWindow().GetContentScale(contentScaleX, contentScaleY);

    context.world = &scene.GetWorld();
    context.scene = &scene;
    context.engine = &engineContext;
    context.selection = &selection;
    context.commandStack = &commandStack;
    context.viewport = &viewport;
    context.textureCatalog = &textureCatalog;
    context.contentModel = &contentModel;
    context.mode = mode;
    context.statusLine = statusLine;

    ProcessUiCanvasesInput(scene, engineContext.GetInput(), fbW, fbH, contentScaleX, contentScaleY);

    PumpSceneLoads(scene.GetWorld());

    const bool editingEnabled = !playSession.IsActive();
    if (editingEnabled) {
        HandleEditModeInput(engineContext);
    }
    HandlePlayModeInput(engineContext, scene.GetWorld());

    const Ui::Rect uiViewport = ComputeUiCanvasViewport(fbW, fbH);
    dock.SyncLayout(uiViewport);
    dock.SyncGizmoToolbar();

    const Ui::Rect worldViewport = dock.GetWorldViewportRect();
    viewport.Simulate(
            timing,
            scene,
            engineContext,
            selection,
            worldViewport,
            fbW,
            fbH,
            statusLine,
            editingEnabled);

    TickPanels(timing, scene, engineContext);
    if (fpsText != nullptr) {
        fpsText->SetText(statusLine);
    }
}

void EditorApplication::OnRender(Scene& scene, IEngineContext& engineContext) {
    HighlightSelection(scene);

    int fbW = 0;
    int fbH = 0;
    engineContext.GetFramebufferSize(fbW, fbH);

    context.world = &scene.GetWorld();
    context.scene = &scene;
    context.engine = &engineContext;
    context.selection = &selection;
    context.commandStack = &commandStack;
    context.viewport = &viewport;
    context.textureCatalog = &textureCatalog;
    context.contentModel = &contentModel;
    context.mode = mode;
    context.statusLine = statusLine;

    const Ui::Rect uiViewport = ComputeUiCanvasViewport(fbW, fbH);
    dock.SyncLayout(uiViewport);
    dock.SyncGizmoToolbar();

    if (inspectorPanel != nullptr) {
        inspectorPanel->PrepareForFrame(context);
    }

    SceneRenderParams params{};
    params.uiFont = scene.GetWorld().GetUiFont();
    params.uiBoldFont = scene.GetWorld().GetUiBoldFont();
    params.screenTexts.Clear();
    params.uiPaintOrderNext = 0U;
    PaintUiCanvases(scene, params, fbW, fbH);

    if (inspectorPanel != nullptr) {
        inspectorPanel->OnPostPaint(context);
    }

    const Ui::Rect worldViewport = dock.GetWorldViewportRect();
    float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
    if (worldViewport.width > 1.0F && worldViewport.height > 1.0F) {
        aspect = worldViewport.width / worldViewport.height;
        params.worldViewportScissorEnabled = true;
        params.worldViewportScissorX = worldViewport.x;
        params.worldViewportScissorY = worldViewport.y;
        params.worldViewportScissorW = worldViewport.width;
        params.worldViewportScissorH = worldViewport.height;
    }

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.1F, 500.0F);
    const Matrix4 view = viewport.GetCamera().ViewMatrix();
    const Matrix4 viewProj = proj * view;

    const Vector3 lightDir = Vector3{0.35F, 0.88F, 0.32F}.Normalized();
    FillStandardLitSceneFromWorld(
            scene.GetWorld(),
            engineContext,
            viewProj,
            viewport.GetCamera().position,
            lightDir,
            Vector3{1.0F, 0.98F, 0.94F},
            0.95F,
            Vector3{0.11F, 0.12F, 0.14F},
            false,
            Vector3{1.0F, 0.0F, 0.0F},
            Vector3{0.0F, 1.0F, 0.0F},
            0.0F,
            params,
            SceneSpriteSortMode::SortOrderOnly,
            &scene);

    if (!playSession.IsActive()) {
        viewport.AppendGizmoDraws(params, selection);
    }

    scene.ForEachTextOverlay([&params](const TextOverlayComponent& tc) {
        ScreenTextDraw draw{};
        draw.text = tc.GetText();
        draw.x = tc.GetScreenX();
        draw.y = tc.GetScreenY();
        draw.sizePixels = tc.GetFontSizePixels();
        draw.color = tc.GetColor();
        draw.alpha = tc.GetAlpha();
        draw.paintOrder = params.NextUiPaintOrder();
        params.screenTexts.PushBack(MoveTemp(draw));
    });

    engineContext.SetSceneRenderParams(params);
}

}  // namespace Spark::Editor
