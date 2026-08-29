#include "spark/editor/EditorApplication.hpp"

#include "spark/config.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
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
#include "spark/scene/core/SceneEntityRole.hpp"
#include "spark/scene/prefab/PrefabInstantiator.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/platform/NativeFilePicker.hpp"

#include <GLFW/glfw3.h>

#if SPARK_ENABLE_IMGUI
#include <imgui.h>
#endif

#include <cstring>
#include <filesystem>
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
    bootstrapGround = ground;

    GameObject* sun = world.CreateGameObject();
    sun->GetName() = Utf8String("Sun");
    TransformComponent* sunTr = sun->AddComponent<TransformComponent>();
    sunTr->SetTranslation({18.0F, 28.0F, 12.0F});
    sun->AddComponent<PointLightComponent>(Vector3{1.0F, 0.96F, 0.9F}, 2.4F, 120.0F);
    contentModel.TrackRoot(sun);
    bootstrapSun = sun;

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
    commandStack.SetOnChanged(&EditorApplication::OnCommandStackChangedStatic, this);
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
    projectLocationUserSet = false;
    ApplyOpenProject(world);

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

void EditorApplication::OnCommandStackChangedStatic(void* userData) noexcept {
    if (userData != nullptr) {
        static_cast<EditorApplication*>(userData)->MarkSceneDirty();
    }
}

void EditorApplication::OnSelectionChanged() {
    if (GameObject* const primary = selection.GetPrimary()) {
        statusLine = primary->GetName();
    } else {
        statusLine = Utf8String("Selection cleared.");
    }
    if (hierarchyPanel != nullptr) {
        hierarchyPanel->SyncToSelection();
    }
    UpdateStatusHud();
}

void EditorApplication::MarkSceneDirty() noexcept {
    sceneDirty = true;
    UpdateStatusHud();
}

void EditorApplication::ClearSceneDirty() noexcept {
    sceneDirty = false;
    UpdateStatusHud();
}

void EditorApplication::UpdateStatusHud() noexcept {
    if (fpsText == nullptr) {
        return;
    }
    Utf8String display{};
    if (sceneDirty) {
        display = Utf8String("* ");
    }
    display.AppendUtf8(statusLine.CStr());
    fpsText->SetText(display);
}

void EditorApplication::SetStatusFromPlaySession(const char* message, void* userData) noexcept {
    if (userData != nullptr && message != nullptr) {
        static_cast<EditorApplication*>(userData)->statusLine = Utf8String(message);
    }
}

Utf8String EditorApplication::GetEditorAssetsRoot() const noexcept {
    if (project.IsOpen()) {
        const Utf8String assetsRoot = project.GetAssetsRootAbsolute();
        if (!assetsRoot.IsEmpty()) {
            return assetsRoot;
        }
    }
    return Utf8String(ScenePathResolver::BuildAssetsRoot());
}

Utf8String EditorApplication::JoinProjectAssetPath(const char* const relativePath) const noexcept {
    if (relativePath == nullptr || relativePath[0] == '\0') {
        return {};
    }
    const char* rel = relativePath;
    while (rel[0] == '/') {
        ++rel;
    }
    const Utf8String assetsRoot = GetEditorAssetsRoot();
    if (assetsRoot.IsEmpty()) {
        return {};
    }
    char buffer[1024]{};
    const int written = std::snprintf(buffer, sizeof(buffer), "%s/%s", assetsRoot.CStr(), rel);
    if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(buffer)) {
        return {};
    }
    return Utf8String(buffer);
}

Utf8String EditorApplication::ResolveEditorScenePath(const char* const relativeOrAbsolutePath) const noexcept {
    if (relativeOrAbsolutePath == nullptr || relativeOrAbsolutePath[0] == '\0') {
        return {};
    }
    if (ScenePathResolver::FileExists(relativeOrAbsolutePath)) {
        return Utf8String(relativeOrAbsolutePath);
    }
    const Utf8String projectPath = JoinProjectAssetPath(relativeOrAbsolutePath);
    if (!projectPath.IsEmpty() && ScenePathResolver::FileExists(projectPath.CStr())) {
        return projectPath;
    }
    return ScenePathResolver::ResolveReadablePath(relativeOrAbsolutePath);
}

bool EditorApplication::EnsureParentDirectoryExists(const char* const filePath) noexcept {
    if (filePath == nullptr || filePath[0] == '\0') {
        return false;
    }
    std::error_code ec{};
    const std::filesystem::path parent = std::filesystem::path(filePath).parent_path();
    if (parent.empty()) {
        return true;
    }
    std::filesystem::create_directories(parent, ec);
    return !ec;
}

bool EditorApplication::ShouldCaptureSceneEntity(const GameObject* const object) const noexcept {
    if (object == nullptr || object == bootstrapGround || object == bootstrapSun) {
        return false;
    }
    const Utf8String& name = object->GetName();
    if (name == Utf8String("EditorGui") || name == Utf8String("EditorStatusHud")) {
        return false;
    }
    if (contentModel.IsInsidePlacedPrefab(object)) {
        return false;
    }
    return contentModel.ShouldCapture(object);
}

void EditorApplication::PaintFileMenuBar() {
#if SPARK_ENABLE_IMGUI
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("New Project…")) {
            pendingFileDialog = PendingFileDialog::NewProject;
        }
        if (ImGui::MenuItem("Open Project…")) {
            pendingFileDialog = PendingFileDialog::OpenProject;
        }
        if (ImGui::MenuItem("Open Scene…")) {
            pendingFileDialog = PendingFileDialog::OpenScene;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
            pendingFileDialog = PendingFileDialog::SaveScene;
        }
        if (ImGui::MenuItem("Save Scene As…")) {
            pendingFileDialog = PendingFileDialog::SaveSceneAs;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Save Project")) {
            pendingFileDialog = PendingFileDialog::SaveProject;
        }
        if (ImGui::MenuItem("Save Project As…")) {
            pendingFileDialog = PendingFileDialog::SaveProjectAs;
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
#endif
}

void EditorApplication::ProcessPendingFileDialogs() {
    if (pendingFileDialog == PendingFileDialog::None) {
        return;
    }
    const PendingFileDialog action = pendingFileDialog;
    pendingFileDialog = PendingFileDialog::None;
    switch (action) {
    case PendingFileDialog::NewProject:
        NewProject();
        break;
    case PendingFileDialog::OpenProject:
        OpenProject();
        break;
    case PendingFileDialog::OpenScene:
        OpenScene();
        break;
    case PendingFileDialog::SaveProject:
        SaveProject();
        break;
    case PendingFileDialog::SaveProjectAs:
        SaveProjectAs();
        break;
    case PendingFileDialog::SaveScene:
        SaveScene(false);
        break;
    case PendingFileDialog::SaveSceneAs:
        SaveScene(true);
        break;
    default:
        break;
    }
}

void EditorApplication::SaveEditorLayout() noexcept {
    Ui::SceneEditorLayoutSettings layout{};
    (void)Ui::TryLoadSceneEditorLayout(layout);
    dock.CaptureLayoutSettings(layout);
    layout.guiTheme = Ui::GetActiveUiThemePreset();
    (void)Ui::SaveSceneEditorLayout(layout);
}

void EditorApplication::ApplyOpenProject(GameWorld& world) {
    const Utf8String assetsRoot = project.GetAssetsRootAbsolute();
    assetCatalog.SetAssetsRootOverride(assetsRoot.IsEmpty() ? nullptr : assetsRoot.CStr());
    assetCatalog.Refresh();
    textureCatalog.Refresh();
    if (projectPanel != nullptr) {
        projectPanel->GetAssetBrowser().RefreshListFromCatalog();
    }

    const Utf8String& mainScene = project.GetSettings().mainScenePath;
    if (!mainScene.IsEmpty()
            && ScenePathResolver::FileExists(ResolveEditorScenePath(mainScene.CStr()).CStr())) {
        LoadSceneFromPath(world, mainScene.CStr());
    }
}

void EditorApplication::NewProject() {
    if (playSession.IsActive() || context.world == nullptr) {
        return;
    }
    Utf8String picked{};
    if (!NativeFilePicker::TryPickProjectFolder(picked)) {
        return;
    }
    if (!project.CreateNewAt(picked.CStr(), workspace)) {
        statusLine = Utf8String("Could not create project.");
        UpdateStatusHud();
        return;
    }
    projectLocationUserSet = true;
    ApplyOpenProject(*context.world);
    statusLine = Utf8String("New project created.");
    UpdateStatusHud();
}

void EditorApplication::OpenProject() {
    if (playSession.IsActive() || context.world == nullptr) {
        return;
    }
    Utf8String picked{};
    if (!NativeFilePicker::TryPickProjectFolder(picked)) {
        return;
    }
    if (!project.OpenExisting(picked.CStr())) {
        statusLine = Utf8String("Could not open project.");
        UpdateStatusHud();
        return;
    }
    projectLocationUserSet = true;
    ApplyOpenProject(*context.world);
    statusLine = Utf8String("Project opened.");
    UpdateStatusHud();
}

void EditorApplication::SaveProject() {
    if (!project.IsOpen()) {
        statusLine = Utf8String("No project open.");
        UpdateStatusHud();
        return;
    }
    if (!projectLocationUserSet) {
        SaveProjectAs();
        return;
    }
    if (!project.TrySaveProjectFile()) {
        statusLine = Utf8String("Project save failed.");
        UpdateStatusHud();
        return;
    }
    statusLine = Utf8String("Project saved.");
    UpdateStatusHud();
}

void EditorApplication::SaveProjectAs() {
    if (playSession.IsActive()) {
        return;
    }
    Utf8String picked{};
    if (!NativeFilePicker::TryPickSaveProjectFolder(picked)) {
        return;
    }
    if (!project.RelocateRoot(picked.CStr())) {
        statusLine = Utf8String("Invalid project folder.");
        UpdateStatusHud();
        return;
    }
    if (!project.TrySaveProjectFile()) {
        statusLine = Utf8String("Project save failed.");
        UpdateStatusHud();
        return;
    }
    projectLocationUserSet = true;
    if (context.world != nullptr) {
        const Utf8String assetsRoot = project.GetAssetsRootAbsolute();
        assetCatalog.SetAssetsRootOverride(assetsRoot.IsEmpty() ? nullptr : assetsRoot.CStr());
        assetCatalog.Refresh();
        if (projectPanel != nullptr) {
            projectPanel->GetAssetBrowser().RefreshListFromCatalog();
        }
    }
    statusLine = Utf8String("Project saved.");
    UpdateStatusHud();
}

void EditorApplication::OpenScene() {
    if (playSession.IsActive() || context.world == nullptr) {
        return;
    }
    Utf8String defaultDirectory = JoinProjectAssetPath("scenes");
    if (defaultDirectory.IsEmpty() && !activeSceneAbsolutePath.IsEmpty()) {
        defaultDirectory = Utf8String(std::filesystem::path(activeSceneAbsolutePath.CStr()).parent_path().c_str());
    }
    Utf8String picked{};
    if (!NativeFilePicker::TryPickOpenSparkSceneFile(
                picked,
                defaultDirectory.IsEmpty() ? nullptr : defaultDirectory.CStr())) {
        statusLine = Utf8String("Open scene cancelled.");
        UpdateStatusHud();
        return;
    }
    activeSceneAbsolutePath = picked;
    activeSceneRelativePath.Clear();

    SceneDocument document;
    SceneDeserializer deserializer;
    if (!deserializer.ReadFromFile(picked.CStr(), document)) {
        statusLine = Utf8String("Invalid scene file.");
        UpdateStatusHud();
        return;
    }
    UnloadEditorSceneContent(*context.world);
    pendingLoadDocument = document;
    SceneLoadOptions options{};
    const Utf8String assetsRoot = GetEditorAssetsRoot();
    options.assetsRoot = assetsRoot.CStr();
    options.additive = true;
    loadedSceneId = sceneManager->BeginLoadSceneAsync(document, picked.CStr(), options);
    if (loadedSceneId == kInvalidSceneInstanceId) {
        pendingLoadDocument = SceneDocument{};
        statusLine = Utf8String("Scene load failed to start.");
        UpdateStatusHud();
        return;
    }
    sceneLoadInProgress = true;
    selection.Clear();
    if (project.IsOpen()) {
        const Utf8String projectAssetsRoot = project.GetAssetsRootAbsolute();
        if (!projectAssetsRoot.IsEmpty()) {
            const char* abs = picked.CStr();
            const char* root = projectAssetsRoot.CStr();
            const std::size_t rootLen = std::strlen(root);
            if (std::strncmp(abs, root, rootLen) == 0) {
                const char* tail = abs + rootLen;
                while (tail[0] == '/') {
                    ++tail;
                }
                if (tail[0] != '\0') {
                    activeSceneRelativePath = Utf8String(tail);
                }
            }
        }
    }
    if (sceneManager->IsSceneReady(loadedSceneId)) {
        FinalizeAsyncSceneLoad(*context.world);
        ClearSceneDirty();
        statusLine = Utf8String("Scene loaded.");
        sceneLoadInProgress = false;
    } else {
        statusLine = Utf8String("Loading scene…");
    }
    UpdateStatusHud();
}

void EditorApplication::SaveScene(const bool forcePicker) {
    if (playSession.IsActive() || context.world == nullptr) {
        return;
    }
    SaveSceneToFile(*context.world, forcePicker);
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

void EditorApplication::UnloadEditorSceneContent(GameWorld& world) {
    if (sceneManager) {
        instanceTracker.UnloadAll(*sceneManager);
    }
    instanceTracker.Clear();
    loadedSceneId = kInvalidSceneInstanceId;
    sceneLoadInProgress = false;
    pendingLoadDocument = SceneDocument{};
    selection.Clear();

    contentModel.ClearManualObjects(world);
    contentModel.ClearLists();
    EnsureEditorBootstrapScene(world);
}

void EditorApplication::EnsureEditorBootstrapScene(GameWorld& world) {
    auto objectStillAlive = [&](const GameObject* candidate) -> bool {
        if (candidate == nullptr) {
            return false;
        }
        bool alive = false;
        world.ForEachGameObject([&](const GameObject* obj) {
            if (obj == candidate) {
                alive = true;
            }
        });
        return alive;
    };

    if (!objectStillAlive(bootstrapGround)) {
        GameObject* ground = world.CreateGameObject();
        ground->GetName() = Utf8String("Ground");
        ground->AddComponent<TransformComponent>();
        ground->AddComponent<MeshComponent>(
                groundMesh, SceneMeshSlot::GroundPlane, Vector3{0.45F, 0.48F, 0.5F});
        bootstrapGround = ground;
    }
    if (!objectStillAlive(bootstrapSun)) {
        GameObject* sun = world.CreateGameObject();
        sun->GetName() = Utf8String("Sun");
        TransformComponent* sunTr = sun->AddComponent<TransformComponent>();
        sunTr->SetTranslation({18.0F, 28.0F, 12.0F});
        sun->AddComponent<PointLightComponent>(Vector3{1.0F, 0.96F, 0.9F}, 2.4F, 120.0F);
        bootstrapSun = sun;
    }
}

void EditorApplication::FinalizeAsyncSceneLoad(GameWorld& world) {
    if (loadedSceneId == kInvalidSceneInstanceId) {
        return;
    }
    SceneEditorContentBindingHooks hooks{};
    hooks.unitCubeAsset = &unitCubeAsset;
    contentModel.IntegrateLoadedInstance(world, loadedSceneId, pendingLoadDocument, hooks);

    const Array<GameObject*> instanceObjects =
            SceneEntityRoleClassifier::CollectInstanceObjects(world, loadedSceneId);
    for (std::size_t i = 0; i < instanceObjects.GetSize(); ++i) {
        GameObject* object = instanceObjects[i];
        if (object == nullptr) {
            continue;
        }
        if (object->GetParent() == nullptr) {
            bool trackedRoot = false;
            const Array<GameObject*>& roots = contentModel.GetRoots();
            for (std::size_t ri = 0; ri < roots.GetSize(); ++ri) {
                if (roots[ri] == object) {
                    trackedRoot = true;
                    break;
                }
            }
            if (!trackedRoot) {
                contentModel.TrackRoot(object);
            }
        }
        if (object->GetComponent<MeshComponent>() != nullptr && !contentModel.IsInsidePlacedPrefab(object)
                && contentModel.FindPlacedOwner(object) == nullptr) {
            contentModel.TrackPlaced(object, contentModel.LookupMeshAssetRel(*object));
        }
        if ((object->GetComponent<PointLightComponent>() != nullptr || object->GetComponent<SpotLightComponent>() != nullptr)) {
            bool trackedLight = false;
            const Array<GameObject*>& lights = contentModel.GetUserLights();
            for (std::size_t li = 0; li < lights.GetSize(); ++li) {
                if (lights[li] == object) {
                    trackedLight = true;
                    break;
                }
            }
            if (!trackedLight) {
                contentModel.TrackUserLight(object);
            }
        }
    }

    instanceTracker.Register(loadedSceneId);
    selection.Clear();
    ClearSceneDirty();
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
        ClearSceneDirty();
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

void EditorApplication::SaveSceneToFile(GameWorld& world, const bool forcePicker) {
    if (inspectorPanel != nullptr) {
        inspectorPanel->CommitAllPendingEdits(context);
    }

    Utf8String absolutePath = activeSceneAbsolutePath;
    if (!forcePicker && absolutePath.IsEmpty() && !activeSceneRelativePath.IsEmpty()) {
        absolutePath = ResolveEditorScenePath(activeSceneRelativePath.CStr());
    }

    const char* suggestedName = "editor_session.sparkscene";
    if (!activeSceneRelativePath.IsEmpty()) {
        const char* slash = std::strrchr(activeSceneRelativePath.CStr(), '/');
        suggestedName = slash != nullptr ? slash + 1 : activeSceneRelativePath.CStr();
    }

    if (forcePicker || absolutePath.IsEmpty()) {
        if (!forcePicker && project.IsOpen()) {
            Utf8String defaultRel = project.GetSettings().mainScenePath;
            if (defaultRel.IsEmpty()) {
                defaultRel = Utf8String("scenes/editor_session.sparkscene");
            }
            const Utf8String defaultAbs = JoinProjectAssetPath(defaultRel.CStr());
            if (!defaultAbs.IsEmpty()) {
                absolutePath = defaultAbs;
                activeSceneRelativePath = defaultRel;
                activeSceneAbsolutePath = absolutePath;
            }
        }
    }
    if (forcePicker || absolutePath.IsEmpty()) {
        Utf8String defaultDirectory = JoinProjectAssetPath("scenes");
        if (defaultDirectory.IsEmpty() && !absolutePath.IsEmpty()) {
            defaultDirectory =
                    Utf8String(std::filesystem::path(absolutePath.CStr()).parent_path().c_str());
        }
        Utf8String picked{};
        if (!NativeFilePicker::TryPickSaveSparkSceneFile(
                    picked,
                    suggestedName,
                    defaultDirectory.IsEmpty() ? nullptr : defaultDirectory.CStr())) {
            statusLine = Utf8String("Save scene cancelled.");
            UpdateStatusHud();
            return;
        }
        absolutePath = MoveTemp(picked);
        activeSceneAbsolutePath = absolutePath;
        activeSceneRelativePath.Clear();
    }

    SceneCaptureContext captureCtx = contentModel.BuildCaptureContext();
    const auto includeEntity = [this](const GameObject* go) -> bool { return ShouldCaptureSceneEntity(go); };

    SceneSerializer serializer;
    SceneDocument document = serializer.Capture(world, captureCtx, includeEntity);
    if (document.entities.IsEmpty()) {
        statusLine = Utf8String("Nothing to save (no tracked scene content).");
        UpdateStatusHud();
        return;
    }
    document.header.name = Utf8String("EditorSession");
    if (project.IsOpen()) {
        document.header.assetsRoot = project.GetSettings().assetsDirectory;
        if (document.header.assetsRoot.IsEmpty()) {
            document.header.assetsRoot = Utf8String("assets");
        }
    } else {
        document.header.assetsRoot = Utf8String("assets");
    }
    if (!EnsureParentDirectoryExists(absolutePath.CStr())) {
        statusLine = Utf8String("Save failed (could not create folder).");
        UpdateStatusHud();
        return;
    }
    if (!serializer.WriteToFile(document, absolutePath.CStr())) {
        statusLine = Utf8String("Save failed (could not open file).");
        UpdateStatusHud();
        return;
    }
    activeSceneAbsolutePath = absolutePath;
    if (project.IsOpen()) {
        const Utf8String assetsRoot = project.GetAssetsRootAbsolute();
        if (!assetsRoot.IsEmpty()) {
            const char* abs = absolutePath.CStr();
            const char* root = assetsRoot.CStr();
            const std::size_t rootLen = std::strlen(root);
            if (std::strncmp(abs, root, rootLen) == 0) {
                const char* tail = abs + rootLen;
                while (tail[0] == '/') {
                    ++tail;
                }
                if (tail[0] != '\0') {
                    activeSceneRelativePath = Utf8String(tail);
                }
            }
        }
        assetCatalog.Refresh();
        if (projectPanel != nullptr) {
            projectPanel->GetAssetBrowser().RefreshListFromCatalog();
        }
    }
    ClearSceneDirty();
    statusLine = Utf8String(
            std::format("Saved {} entities → {}", document.entities.GetSize(), absolutePath.CStr()).c_str());
    UpdateStatusHud();
}

void EditorApplication::LoadSceneFromPath(GameWorld& world, const char* relativeScenePath) {
    if (relativeScenePath == nullptr || relativeScenePath[0] == '\0' || !sceneManager) {
        statusLine = Utf8String("Invalid scene path.");
        return;
    }
    const Utf8String path = ResolveEditorScenePath(relativeScenePath);
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
    UnloadEditorSceneContent(world);
    pendingLoadDocument = document;
    SceneLoadOptions options{};
    const Utf8String assetsRoot = GetEditorAssetsRoot();
    options.assetsRoot = assetsRoot.CStr();
    options.additive = true;
    loadedSceneId = sceneManager->BeginLoadSceneAsync(document, path.CStr(), options);
    if (loadedSceneId == kInvalidSceneInstanceId) {
        pendingLoadDocument = SceneDocument{};
        statusLine = Utf8String("Scene load failed to start.");
        return;
    }
    sceneLoadInProgress = true;
    selection.Clear();
    activeSceneRelativePath = Utf8String(relativeScenePath);
    activeSceneAbsolutePath = path;
    if (sceneManager->IsSceneReady(loadedSceneId)) {
        FinalizeAsyncSceneLoad(world);
        ClearSceneDirty();
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
    const Utf8String path = ResolveEditorScenePath(entry.relativePath.CStr());
    const Vector3 groundHit = viewport.GetLastGroundHit();
    PrefabInstantiateOptions options{};
    const Utf8String assetsRoot = GetEditorAssetsRoot();
    options.assetsRoot = assetsRoot.CStr();
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
    MarkSceneDirty();
    statusLine = Utf8String("Placed prefab.");
    UpdateStatusHud();
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

void EditorApplication::HandleEditModeInput(IEngineContext& context, GameWorld& world) {
    IInput& input = context.GetInput();
    const bool ctrlDown = input.IsKeyDown(GLFW_KEY_LEFT_CONTROL) || input.IsKeyDown(GLFW_KEY_RIGHT_CONTROL);

    if (ctrlDown && input.IsKeyPressedThisFrame(GLFW_KEY_S)) {
        pendingFileDialog = (input.IsKeyDown(GLFW_KEY_LEFT_SHIFT) || input.IsKeyDown(GLFW_KEY_RIGHT_SHIFT))
                ? PendingFileDialog::SaveSceneAs
                : PendingFileDialog::SaveScene;
        return;
    }
    if (ctrlDown && input.IsKeyPressedThisFrame(GLFW_KEY_Z)) {
        if (commandStack.TryUndo()) {
            statusLine = Utf8String("Undo.");
            UpdateStatusHud();
        }
    }
    if (ctrlDown && input.IsKeyPressedThisFrame(GLFW_KEY_Y)) {
        if (commandStack.TryRedo()) {
            statusLine = Utf8String("Redo.");
            UpdateStatusHud();
        }
    }
}

void EditorApplication::HandlePlayModeInput(IEngineContext& engineContext, GameWorld& world) {
    IInput& input = engineContext.GetInput();
    if (input.IsKeyPressedThisFrame(GLFW_KEY_P) && !playSession.IsActive()) {
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
    SaveEditorLayout();
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
        HandleEditModeInput(engineContext, scene.GetWorld());
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

    ProcessPendingFileDialogs();
    UpdateStatusHud();
}

void EditorApplication::OnRender(Scene& scene, IEngineContext& engineContext) {
    HighlightSelection(scene);

    PaintFileMenuBar();

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
