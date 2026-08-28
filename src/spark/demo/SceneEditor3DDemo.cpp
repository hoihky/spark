#include "spark/demo/SceneEditor3DDemo.hpp"
#include "spark/demo/SceneEditor3DDemo_detail.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"

#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
#include "spark/ui/runtime/EditorLayoutStore.hpp"
#include "spark/ui/runtime/UiContextMenu.hpp"
#include "spark/ui/runtime/UiScene.hpp"
#include "spark/scene/mesh/MeshRaycast.hpp"
#include "spark/scene/query/SceneRaycast.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"
#include "spark/scene/material/MaterialAsset.hpp"
#include "spark/scene/assets/ScenePathResolver.hpp"
#include "spark/scene/core/SceneEntityRole.hpp"
#include "spark/scene/assets/CachedAssetKind.hpp"
#include "spark/scene/assets/AssetLoadEvents.hpp"
#include "spark/scene/prefab/PrefabInstantiator.hpp"
#include "spark/config.hpp"

namespace Spark {

void SceneEditor3DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        selectedObject = nullptr;
        dragPlaced = nullptr;
        transformGizmo.EndDrag();
        cameraController.EndOrbitDrag();
        rmbDragDistSq = 0.0F;
        selectionPulseTime = 0.0F;
        lightEditTarget = nullptr;
        statusMessage.Clear();
        contentModel.ClearLists();
        instanceTracker.Clear();
        placementActions.Clear();
        prefabCatalog.Clear();
        unitCubeAsset.Reset();
        groundAsset.Reset();
        loadedSceneId = Spark::kInvalidSceneInstanceId;
        pendingLoadDocument = SceneDocument{};
        sceneLoadInProgress = false;
        loadSession.Reset();
        sceneManager.Reset();
        Spark::Ui::GetUiContextMenu().Close();

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SceneEditorUnitCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        w.RegisterMesh(unitCubeAsset, "spark/scene_editor/unit_cube");

        groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SceneEditorGround"));
        *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);
        w.RegisterMesh(groundAsset, "spark/scene_editor/ground");

        Spark::GameObject* ground = w.CreateGameObject();
        ground->GetName() = Spark::Utf8String("SceneEditorGround");
        ground->AddComponent<Spark::TransformComponent>();
        ground->AddComponent<Spark::MeshComponent>(
                groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3{0.48F, 0.52F, 0.55F});
        contentModel.TrackRoot(ground);

        Spark::GameObject* sun = w.CreateGameObject();
        sun->GetName() = Spark::Utf8String("SceneEditorSun");
        Spark::TransformComponent* str = sun->AddComponent<Spark::TransformComponent>();
        str->SetTranslation({24.0F, 38.0F, 18.0F});
        const Spark::Vector3 sunDir = Spark::Vector3{0.35F, 0.82F, 0.38F}.Normalized();
        str->SetRotation(Spark::Quaternion::FromShortestArc(Spark::Vector3::UnitZ, sunDir));
        sun->AddComponent<Spark::DirectionalLightComponent>(Spark::Vector3{1.0F, 0.97F, 0.92F}, 0.92F);
        contentModel.TrackRoot(sun);

        SetupContextMenuCanvas(w);
        assetCatalog.Refresh();
        assetBrowser.Mount(w, assetCatalog, *this);

        helpHud.Mount(w, "Scene editor");
        helpHud.SetControlHints(
                "W/E/R gizmo · F focus · Home reset · asset panel left · RMB menu · I import · P play · Esc stop");

        sceneManager = Spark::MakeUnique<Spark::SceneManager>(w);
        loadSession = Spark::MakeUnique<Spark::SceneLoadSession>(*sceneManager);

        PrefabCatalog::RegisterDemoDefaults(prefabCatalog);
        ScenePlacementActionRegistry::RegisterDemoDefaults(placementActions);
        ScenePlacementActionRegistry::RegisterPrefabActions(placementActions, prefabCatalog);
        gltfImportService.Bind(&prefabCatalog, &placementActions);
        placementActions.Register(MakeLambdaPlacementAction(
                Utf8String("Import GLTF model..."), &SceneEditor3DDemo::PlacementImportGltf));
        placementActions.Register(MakeLambdaPlacementAction(
                Utf8String("Delete selected"), &SceneEditor3DDemo::PlacementDeleteSelected, &SceneEditor3DDemo::PlacementDeleteAvailable));
        placementActions.Register(
                MakeLambdaPlacementAction(Utf8String("Save scene"), &SceneEditor3DDemo::PlacementSaveScene));
        placementActions.Register(
                MakeLambdaPlacementAction(Utf8String("Load scene"), &SceneEditor3DDemo::PlacementLoadScene));

        context.GetInput().SetCursorCaptured(false);
        cameraController.ResetToDefault();
        transformGizmo.SetMode(TransformGizmoMode::Translate);

        const Utf8String arenaPath = ScenePathResolver::BuildRuntimePath("scenes", "arena.sparkscene");
        if (ScenePathResolver::FileExists(arenaPath.CStr())) {
            LoadSceneFromFile(w);
        }
    }

void SceneEditor3DDemo::Unload(Spark::GameWorld& w)
{
        helpHud.Unmount(w);
        assetBrowser.Unmount(w);
        Spark::Ui::GetUiContextMenu().Close();
        if (sceneManager) {
            UnloadEditorSceneContent(w);
        }
        for (std::size_t i = 0; i < contentModel.GetRoots().GetSize(); ++i) {
            if (contentModel.GetRoots()[i] != nullptr) {
                w.DestroyGameObject(contentModel.GetRoots()[i]);
            }
        }
        contentModel.GetRoots().Clear();
        loadSession.Reset();
        sceneManager.Reset();
        lightEditTarget = nullptr;
        statusMessage.Clear();
        cameraController.EndOrbitDrag();
        transformGizmo.EndDrag();
        selectedObject = nullptr;
        unitCubeAsset.Reset();
        groundAsset.Reset();
        editorWorld_ = nullptr;
    }

void SceneEditor3DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
        editorWorld_ = &world;
        if (sceneManager) {
            sceneManager->Pump();
            if (sceneLoadInProgress && loadedSceneId != Spark::kInvalidSceneInstanceId) {
                if (sceneManager->IsSceneReady(loadedSceneId)) {
                    FinalizeAsyncSceneLoad(world);
                    sceneLoadInProgress = false;
                } else if (sceneManager->HasSceneFailed(loadedSceneId)) {
                    sceneManager->UnloadScene(loadedSceneId);
                    loadedSceneId = Spark::kInvalidSceneInstanceId;
                    sceneLoadInProgress = false;
                    pendingLoadDocument = SceneDocument{};
                    SetStatusMessage(Spark::Utf8String("Scene load failed (missing assets?)."));
                }
            }
        }

        Spark::IInput& in = context.GetInput();
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        float mx = 0.0F;
        float my = 0.0F;
        in.GetCursorFramebufferPixels(mx, my, fbW, fbH);
        const bool inViewport = IsPointerInEditorViewport(mx, fbW);

        Spark::ProcessUiCanvasesInput(world, in, fbW, fbH);

        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_W) && !playSession.IsActive()) {
            transformGizmo.SetMode(TransformGizmoMode::Translate);
            SetStatusMessage(Spark::Utf8String("Gizmo: Move (W/E/R to switch)."));
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_E) && !playSession.IsActive()) {
            transformGizmo.SetMode(TransformGizmoMode::Rotate);
            SetStatusMessage(Spark::Utf8String("Gizmo: Rotate (W/E/R to switch)."));
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_R) && !playSession.IsActive()) {
            transformGizmo.SetMode(TransformGizmoMode::Scale);
            SetStatusMessage(Spark::Utf8String("Gizmo: Scale (W/E/R to switch)."));
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_Q) && !playSession.IsActive()) {
            transformGizmo.CycleMode();
            SetStatusMessage(Spark::Utf8String(transformGizmo.GetInteractionHint()));
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F) && !playSession.IsActive()) {
            FocusCameraOnSelection();
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_HOME) && !playSession.IsActive()) {
            ResetEditorCamera();
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F9)) {
            TrySaveSelectedMaterial(world);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F10)) {
            TryLoadSelectedMaterial(world);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_I) && !playSession.IsActive()) {
            ScenePlacementContext placementCtx = MakePlacementContext(world, lastGroundHit, selectedObject);
            const GltfImportService::ImportResult importResult =
                    gltfImportService.ImportFromFilePickerAndPlace(world, placementCtx);
            SetStatusMessage(importResult.message);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_P) && !playSession.IsActive()) {
            SceneEditorPlaySession::Dependencies deps{
                    .loadSession = *loadSession,
                    .content = contentModel,
                    .instances = instanceTracker,
                    .camera = cameraController.camera,
                    .orbitPivot = cameraController.orbitPivot,
                    .orbitDistance = cameraController.orbitDistance,
                    .reloadScene = &SceneEditor3DDemo::ReloadSceneForPlayModeStatic,
                    .setStatus = &SceneEditor3DDemo::SetStatusFromPlacement,
                    .userData = this,
            };
            SaveSceneToFile(world);
            playSession.Enter(world, context, deps);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_ESCAPE) && playSession.IsActive()) {
            SceneEditorPlaySession::Dependencies deps{
                    .loadSession = *loadSession,
                    .content = contentModel,
                    .instances = instanceTracker,
                    .camera = cameraController.camera,
                    .orbitPivot = cameraController.orbitPivot,
                    .orbitDistance = cameraController.orbitDistance,
                    .reloadScene = &SceneEditor3DDemo::ReloadSceneForPlayModeStatic,
                    .setStatus = &SceneEditor3DDemo::SetStatusFromPlacement,
                    .userData = this,
            };
            playSession.Exit(world, context, deps);
        }
        if (in.IsCursorCaptured()) {
            cameraController.UpdateFlyNavigation(in, timing);
        } else {
            if (in.IsMouseButtonPressedThisFrame(1) && inViewport) {
                rmbDragDistSq = 0.0F;
            }
            if (in.IsMouseButtonDown(1) && inViewport && timing.frameIndex > 0) {
                const float mdx = in.GetMouseDeltaX();
                const float mdy = in.GetMouseDeltaY();
                rmbDragDistSq += mdx * mdx + mdy * mdy;
            }
            cameraController.UpdateEditorNavigation(in, timing, inViewport, UiConsumesGamePointer());
            if (in.IsMouseButtonReleasedThisFrame(0)) {
                cameraController.EndOrbitDrag();
            }
        }

        if (!in.IsCursorCaptured() && !UiConsumesGamePointer()) {
            const Spark::Matrix4 view = cameraController.camera.ViewMatrix();
            const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
            const Spark::Matrix4 proj =
                    Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
            const Spark::Matrix4 vp = proj * view;
            Spark::Matrix4 invVp{};
            Spark::Vector3 ro{};
            Spark::Vector3 rd{};
            Spark::Vector3 groundHit{};
            bool haveRay = false;
            if (vp.TryInvert(invVp) && TerrainScreenToWorldRay(fbW, fbH, mx, my, invVp, ro, rd)) {
                haveRay = true;
                const float rdLen = rd.Length();
                if (rdLen > 1.0e-6F) {
                    rd = Spark::Vector3{rd.x / rdLen, rd.y / rdLen, rd.z / rdLen};
                }
            }

            if (in.IsMouseButtonReleasedThisFrame(0)) {
                dragPlaced = nullptr;
                transformGizmo.EndDrag();
            }

            const bool haveGround = haveRay && RayIntersectPlaneY(ro, rd, 0.0F, groundHit);
            if (haveGround) {
                lastGroundHit = groundHit;
            }
            const bool altHeld = in.IsKeyDown(GLFW_KEY_LEFT_ALT) || in.IsKeyDown(GLFW_KEY_RIGHT_ALT);

            if (in.IsMouseButtonReleasedThisFrame(1) && inViewport && rmbDragDistSq < 64.0F && haveGround &&
                !playSession.IsActive()) {
                OpenSceneContextMenu(mx, my, groundHit, selectedObject, world);
            }

            if (cameraController.IsOrbitDragging() && in.IsMouseButtonDown(0) && timing.frameIndex > 0) {
                cameraController.UpdateOrbitDrag(in);
            } else if (haveRay && in.IsMouseButtonPressedThisFrame(0) && !playSession.IsActive()) {
                bool handledPress = false;
                if (altHeld && inViewport) {
                    Spark::Vector3 pivot = cameraController.orbitPivot;
                    if (selectedObject != nullptr) {
                        Spark::TransformComponent* selTr = selectedObject->GetComponent<Spark::TransformComponent>();
                        if (selTr != nullptr) {
                            pivot = selTr->GetLocalTransform().translation;
                        }
                    }
                    cameraController.BeginOrbitDrag(pivot);
                    handledPress = true;
                }
                if (!handledPress && selectedObject != nullptr) {
                    Spark::TransformComponent* selTr = selectedObject->GetComponent<Spark::TransformComponent>();
                    if (selTr != nullptr) {
                        const float ext = SelectionGizmoExtent(selectedObject);
                        if (transformGizmo.TryBeginDrag(ro, rd, *selTr, ext, *selectedObject)) {
                            dragPlaced = selectedObject;
                            handledPress = true;
                            SetStatusMessage(Spark::Utf8String(transformGizmo.GetInteractionHint()));
                        }
                    }
                }
                if (!handledPress) {
                    Spark::Vector3 pickHit{};
                    Spark::Scene* pickScene = context.TryGetScene();
                    if (pickScene != nullptr
                            && TryPickEditorRay(*pickScene, ro, rd, true, true, pickHit)) {
                        selectedObject = dragPlaced;
                        if (IsUserLight(selectedObject)) {
                            lightEditTarget = selectedObject;
                        } else {
                            lightEditTarget = nullptr;
                        }
                        SetStatusMessage(Spark::Utf8String("Selected — drag gizmo or hold LMB on ground to slide XZ."));
                    } else if (haveGround) {
                        selectedObject = nullptr;
                        lightEditTarget = nullptr;
                        dragPlaced = nullptr;
                        SetStatusMessage(Spark::Utf8String("Selection cleared."));
                    }
                }
            }

            if (!cameraController.IsOrbitDragging() && haveRay && in.IsMouseButtonDown(0) && dragPlaced != nullptr &&
                transformGizmo.IsDragging()) {
                Spark::TransformComponent* dtr = dragPlaced->GetComponent<Spark::TransformComponent>();
                if (dtr != nullptr) {
                    (void)transformGizmo.UpdateDrag(ro, rd, *dtr);
                }
            } else if (!cameraController.IsOrbitDragging() && haveRay && in.IsMouseButtonDown(0) &&
                       dragPlaced != nullptr && !transformGizmo.IsDragging() &&
                       transformGizmo.GetMode() == TransformGizmoMode::Translate) {
                Spark::Vector3 dragHit{};
                if (RayIntersectPlaneY(ro, rd, dragPlaneY, dragHit)) {
                    Spark::TransformComponent* dtr = dragPlaced->GetComponent<Spark::TransformComponent>();
                    if (dtr != nullptr) {
                        const Spark::Vector3 t = dtr->GetLocalTransform().translation;
                        dtr->SetTranslation({dragHit.x, t.y, dragHit.z});
                    }
                }
            }
        }

        if (selectedObject != nullptr) {
            selectionPulseTime += timing.deltaTimeSeconds;
        } else {
            selectionPulseTime = 0.0F;
        }

        ValidateLightEditTarget();

        std::string hud = std::format(
                "{} meshes · {} lights",
                static_cast<int>(contentModel.GetPlacedObjects().GetSize()),
                static_cast<int>(contentModel.GetUserLights().GetSize()));
        if (!statusMessage.IsEmpty()) {
            hud += " · ";
            hud += statusMessage.CStr();
        }
        helpHud.SetDetail(hud.c_str());
        helpHud.Update(timing, context);
        assetBrowser.SetEnabled(!playSession.IsActive());
    }

void SceneEditor3DDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
        const Spark::Matrix4 view = cameraController.camera.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = cameraController.camera.position;
        params.lightDirectionWorld = Spark::Vector3{0.35F, 0.82F, 0.38F}.Normalized();
        params.lightColor = {1.0F, 0.97F, 0.92F};
        params.lightIntensity = 0.92F;
        params.ambientColor = {0.10F, 0.11F, 0.14F};

        bool appliedDirectionalLight = false;
        scene.ForEachDirectionalLight([&params, &appliedDirectionalLight](
                                                const Spark::DirectionalLightComponent& dl, const Spark::Matrix4& worldMat) {
            if (appliedDirectionalLight) {
                return;
            }
            Spark::Vector3 towardLight = worldMat.TransformVector(Spark::Vector3{0.0F, 0.0F, 1.0F});
            if (towardLight.LengthSquared() < 1.0e-10F) {
                towardLight = Spark::Vector3{0.0F, 1.0F, 0.0F};
            } else {
                towardLight = towardLight.Normalized();
            }
            params.lightDirectionWorld = towardLight;
            params.lightColor = dl.GetColor();
            params.lightIntensity = dl.GetIntensity();
            params.directionalShadowsEnabled = dl.CastsShadow();
            appliedDirectionalLight = true;
        });

        params.draws.Clear();
        params.transparentDraws.Clear();
        params.sceneTextures.Clear();
        params.pointLights.Clear();
        params.sprites.Clear();
        params.screenRects.Clear();
        params.screenTexts.Clear();
        params.screenOverlayRects.Clear();
        params.screenOverlayTexts.Clear();
        params.screenLateRects.Clear();
        params.screenLateTexts.Clear();
        params.uiFont = world.GetUiFont();
        params.uiBoldFont = world.GetUiBoldFont();
        params.draws.Reserve(48);

        scene.ForEachPointLight([&params](const Spark::PointLightComponent& pl, const Spark::Matrix4& worldMat) {
            if (params.pointLights.GetSize() >= Spark::SceneRenderParams::MaxPointLights) {
                return;
            }
            Spark::ScenePointLight gpu{};
            gpu.positionWorld = worldMat.TranslationVector();
            gpu.range = pl.GetRange();
            gpu.color = pl.GetColor();
            gpu.intensity = pl.GetIntensity();
            gpu.castsShadow = pl.CastsShadow();
            params.pointLights.PushBack(gpu);
        });

        auto findOrAddTexture =
                [&params](const Spark::SharedPtr<Spark::Texture2D>& tex, Spark::Vector2* outUvScale, Spark::Vector2* outUvOffset)
                -> std::int32_t {
            return Spark::SceneSubmitDetail::FindOrAddSceneTexture(
                    params, tex, outUvScale, outUvOffset, nullptr);
        };

        auto applySelectionHighlight = [&](Spark::SceneDrawItem& item, Spark::GameObject* obj) {
            if (obj == nullptr || obj != selectedObject) {
                return;
            }
            const float pulse = 0.82F + 0.18F * std::sin(selectionPulseTime * 6.8F);
            const Spark::Vector3 rim{0.22F, 0.78F, 1.0F};
            item.emissiveColor = {
                    std::min(1.0F, item.emissiveColor.x + rim.x * 0.55F),
                    std::min(1.0F, item.emissiveColor.y + rim.y * 0.55F),
                    std::min(1.0F, item.emissiveColor.z + rim.z * 0.55F)};
            item.emissiveIntensity = item.emissiveIntensity + 2.5F * pulse;
            item.roughness = std::max(0.06F, item.roughness * 0.55F);
            item.albedo = {
                    std::min(1.0F, item.albedo.x * 1.06F + 0.03F),
                    std::min(1.0F, item.albedo.y * 1.04F + 0.05F),
                    std::min(1.0F, item.albedo.z * 1.12F + 0.06F)};
        };

        Spark::Array<Spark::SceneDrawItem> drawList;
        drawList.Reserve(32);
        scene.ForEachDrawable([&](Spark::GameObject* obj, const Spark::MeshComponent& mc,
                                     const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            const Spark::MultiMaterialComponent* multiMat =
                    obj != nullptr ? obj->GetComponent<Spark::MultiMaterialComponent>() : nullptr;

            Spark::SceneDrawItem baseItem{};
            baseItem.model = world;
            baseItem.mesh = mc.GetSlot();
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom) {
                baseItem.customMesh = mc.GetMesh();
            }
            baseItem.albedo = mc.GetAlbedo();
            baseItem.textureLayer = -1;

            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom && mc.GetMesh() && multiMat != nullptr &&
                !mc.GetMesh()->GetSubmeshes().IsEmpty()) {
                const std::size_t startCount = drawList.GetSize();
                Spark::SceneSubmitDetail::PushRigidMeshDraws(
                        drawList, baseItem, *mc.GetMesh(), mat, multiMat, params, findOrAddTexture);
                for (std::size_t i = startCount; i < drawList.GetSize(); ++i) {
                    applySelectionHighlight(drawList[i], obj);
                }
                return;
            }

            Spark::SceneDrawItem item = baseItem;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                Spark::SceneSubmitDetail::ApplyAlbedoTexture(
                        item, mat->GetBaseColorTexture(), mat->GetTint(), findOrAddTexture);
            }
            applySelectionHighlight(item, obj);
            drawList.PushBack(item);
        });

        StableSortDrawItems(drawList);
        PartitionSortedDrawItemsIntoSceneParams(drawList, params, cameraController.camera.position);

        if (selectedObject != nullptr) {
            Spark::TransformComponent* selTr = selectedObject->GetComponent<Spark::TransformComponent>();
            if (selTr != nullptr) {
                const float ext = SelectionGizmoExtent(selectedObject);
                Spark::Array<Spark::SceneDrawItem> gizmoDraws;
                gizmoDraws.Reserve(12);
                transformGizmo.AppendDraws(*selTr, ext, gizmoDraws);
                for (std::size_t gi = 0; gi < gizmoDraws.GetSize(); ++gi) {
                    params.draws.PushBack(gizmoDraws[gi]);
                }
            }
        }

        params.uiPaintOrderNext = 0U;
        Spark::PaintUiCanvases(world, params, fbW, fbH);

        scene.ForEachTextOverlay([&params](const Spark::TextOverlayComponent& tc) {
            Spark::ScreenTextDraw d{};
            d.text = tc.GetText();
            d.x = tc.GetScreenX();
            d.y = tc.GetScreenY();
            d.sizePixels = tc.GetFontSizePixels();
            d.color = tc.GetColor();
            d.alpha = tc.GetAlpha();
            d.paintOrder = params.NextUiPaintOrder();
            params.screenTexts.PushBack(Spark::MoveTemp(d));
        });

        helpHud.PatchSceneRenderParams(params, world);
        context.SetSceneRenderParams(params);
    }

void SceneEditor3DDemo::SetStatusMessage(const Spark::Utf8String& msg)
{
        statusMessage = msg;
    }

[[nodiscard]] float SceneEditor3DDemo::SelectionGizmoExtent(Spark::GameObject* go) const noexcept
{
        if (go == nullptr) {
            return 1.0F;
        }
        const Spark::TransformComponent* tr = go->GetComponent<Spark::TransformComponent>();
        if (tr == nullptr) {
            return 1.0F;
        }
        const Spark::Vector3 sc = tr->GetLocalTransform().scale;
        return std::max({sc.x, sc.y, sc.z, 1.0F});
}

void SceneEditor3DDemo::UnloadEditorSceneContent(Spark::GameWorld& w)
{
        if (sceneManager) {
            instanceTracker.UnloadAll(*sceneManager);
        }
        instanceTracker.Clear();
        loadedSceneId = Spark::kInvalidSceneInstanceId;
        sceneLoadInProgress = false;
        pendingLoadDocument = SceneDocument{};
        selectedObject = nullptr;
        dragPlaced = nullptr;
        transformGizmo.EndDrag();
        lightEditTarget = nullptr;
        contentModel.ClearManualObjects(w);
        contentModel.ClearLists();
    }

void SceneEditor3DDemo::ClearPlaced(Spark::GameWorld& w)
{
        UnloadEditorSceneContent(w);
    }

void SceneEditor3DDemo::RemoveEditorSelection(Spark::GameWorld& w, Spark::GameObject* go) noexcept
{
        if (go == nullptr) {
            return;
        }
        contentModel.RemoveTracked(go, w);
        if (selectedObject == go) {
            selectedObject = nullptr;
        }
        if (dragPlaced == go) {
            dragPlaced = nullptr;
        }
        if (lightEditTarget == go) {
            lightEditTarget = nullptr;
        }
        transformGizmo.EndDrag();
    }

void SceneEditor3DDemo::ClearUserLights(Spark::GameWorld& w)
{
        if (selectedObject != nullptr) {
            for (std::size_t i = 0; i < contentModel.GetUserLights().GetSize(); ++i) {
                if (contentModel.GetUserLights()[i] == selectedObject) {
                    selectedObject = nullptr;
                    break;
                }
            }
        }
        dragPlaced = nullptr;
        transformGizmo.EndDrag();
        lightEditTarget = nullptr;
        for (std::size_t i = 0; i < contentModel.GetUserLights().GetSize(); ++i) {
            if (contentModel.GetUserLights()[i] != nullptr) {
                w.DestroyGameObject(contentModel.GetUserLights()[i]);
            }
        }
        contentModel.GetUserLights().Clear();
    }

[[nodiscard]] bool SceneEditor3DDemo::IsUserLight(Spark::GameObject* go) const noexcept
{
        if (go == nullptr) {
            return false;
        }
        for (std::size_t i = 0; i < contentModel.GetUserLights().GetSize(); ++i) {
            if (contentModel.GetUserLights()[i] == go) {
                return true;
            }
        }
        return false;
    }

void SceneEditor3DDemo::ValidateLightEditTarget() noexcept
{
        if (lightEditTarget != nullptr && !IsUserLight(lightEditTarget)) {
            lightEditTarget = nullptr;
        }
    }

void SceneEditor3DDemo::SyncLightGizmoEmissive(Spark::GameObject* go) noexcept
{
        if (go == nullptr) {
            return;
        }
        Spark::PointLightComponent* pl = go->GetComponent<Spark::PointLightComponent>();
        Spark::MaterialComponent* mat = go->GetComponent<Spark::MaterialComponent>();
        if (pl == nullptr || mat == nullptr) {
            return;
        }
        const float glow = std::min(12.0F, pl->GetIntensity() * 1.35F);
        mat->SetEmissive(pl->GetColor(), glow);
    }

[[nodiscard]] Spark::GameObject* SceneEditor3DDemo::AddUserPointLightAt(
            Spark::GameWorld& w,
            const Spark::Vector3& pos,
            const Spark::Vector3& color,
            float intensity,
            float range)
{
        Spark::GameObject* go = w.CreateGameObject();
        go->GetName() = Spark::Utf8String("SceneEditorUserLight");
        Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
        constexpr float kGizmoScale = 0.2F;
        tr->SetTranslation(pos);
        tr->SetUniformScale(kGizmoScale);
        go->AddComponent<Spark::PointLightComponent>(color, intensity, range)->SetCastsShadow(true);
        go->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{1.0F, 1.0F, 1.0F});
        if (Spark::MaterialComponent* m = go->AddComponent<Spark::MaterialComponent>()) {
            m->SetMetallic(0.12F);
            m->SetRoughness(0.35F);
            m->SetEmissive(color, 7.5F);
        }
        SyncLightGizmoEmissive(go);
        contentModel.TrackRoot(go);
        contentModel.TrackUserLight(go);
        return go;
    }

[[nodiscard]] bool SceneEditor3DDemo::TryPickEditorRay(
        Spark::Scene& scene,
        const Spark::Vector3& rayOrigin,
        const Spark::Vector3& rayDir,
        const bool pickMeshes,
        const bool pickLights,
        Spark::Vector3& outHit) noexcept
{
    dragPlaced = nullptr;
    float bestT = 1.0e30F;
    GameObject* bestGo = nullptr;
    Spark::Vector3 bestHit{};

    if (pickMeshes) {
        Spark::SceneRay ray{};
        ray.origin = rayOrigin;
        ray.direction = rayDir;
        ray.tMin = 1.0e-4F;
        ray.tMax = bestT;
        Spark::SceneRaycastHit hit{};
        Spark::SceneRaycastOptions opts{};
        opts.pickSkinnedMeshes = true;
        if (scene.RaycastPick(ray, hit, opts) && hit.object != nullptr) {
            GameObject* placedOwner = contentModel.FindPlacedOwner(hit.object);
            if (placedOwner != nullptr) {
                bestT = hit.distance;
                bestGo = placedOwner;
                bestHit = hit.pointWorld;
            }
        }
    }
    if (pickLights) {
        for (std::size_t j = contentModel.GetUserLights().GetSize(); j > 0U; --j) {
            Spark::GameObject* go = contentModel.GetUserLights()[j - 1U];
            if (go == nullptr) {
                continue;
            }
            Spark::TransformComponent* tr = go->GetComponent<Spark::TransformComponent>();
            if (tr == nullptr) {
                continue;
            }
            const Spark::Vector3 center = tr->GetLocalTransform().translation;
            const float scale = tr->GetLocalTransform().scale.x;
            const float radius = std::max(0.35F, 1.65F * scale);
            float t = 0.0F;
            if (TryRaycastSphereWorld(rayOrigin, rayDir, center, radius, 1.0e-4F, bestT, t)) {
                bestT = t;
                bestGo = go;
                bestHit = {rayOrigin.x + rayDir.x * t, rayOrigin.y + rayDir.y * t, rayOrigin.z + rayDir.z * t};
            }
        }
    }

    if (bestGo != nullptr) {
        dragPlaced = bestGo;
        dragPlaneY = bestHit.y;
        outHit = bestHit;
        return true;
    }
    return false;
}

void SceneEditor3DDemo::SaveSceneToFile(Spark::GameWorld& w)
{
        const Utf8String path = ScenePathResolver::BuildRuntimePath("scenes", "arena.sparkscene");
        SceneCaptureContext ctx = contentModel.BuildCaptureContext();
        const auto includeEntity = [this](const GameObject* go) -> bool { return contentModel.ShouldCapture(go); };

        SceneSerializer serializer;
        SceneDocument document = serializer.Capture(w, ctx, includeEntity);
        document.header.name = Spark::Utf8String("Arena");
        document.header.assetsRoot = Spark::Utf8String(ScenePathResolver::AssetsRoot());
        if (!serializer.WriteToFile(document, path.CStr())) {
            SetStatusMessage(Spark::Utf8String("Save failed (could not open file)."));
            return;
        }
        const std::string saved = std::format(
                "Saved {} entities (spark_scene_v4) → scenes/arena.sparkscene",
                document.entities.GetSize());
        SetStatusMessage(Spark::Utf8String(saved.c_str()));
    }

void SceneEditor3DDemo::FinalizeAsyncSceneLoad(Spark::GameWorld& w)
{
        if (loadedSceneId == Spark::kInvalidSceneInstanceId) {
            return;
        }
        SceneEditorContentBindingHooks hooks{};
        hooks.unitCubeAsset = &unitCubeAsset;
        hooks.syncLightGizmoEmissive = &SceneEditor3DDemo::SyncLightGizmoEmissive;
        contentModel.IntegrateLoadedInstance(w, loadedSceneId, pendingLoadDocument, hooks);
        instanceTracker.Register(loadedSceneId);
        selectedObject = nullptr;
        lightEditTarget = nullptr;
        const std::string loaded = std::format(
                "Loaded {} entities (SceneManager, async assets ready).",
                pendingLoadDocument.entities.GetSize());
        SetStatusMessage(Spark::Utf8String(loaded.c_str()));
        pendingLoadDocument = SceneDocument{};
    }

void SceneEditor3DDemo::LoadSceneFromFile(Spark::GameWorld& w)
{
        const Utf8String path = ScenePathResolver::BuildRuntimePath("scenes", "arena.sparkscene");
        if (!ScenePathResolver::FileExists(path.CStr())) {
            SetStatusMessage(Spark::Utf8String("No save file yet (scenes/arena.sparkscene)."));
            return;
        }
        std::FILE* peek = std::fopen(path.CStr(), "r");
        if (peek == nullptr) {
            SetStatusMessage(Spark::Utf8String("No save file yet (scenes/arena.sparkscene)."));
            return;
        }
        char magic[64]{};
        if (std::fscanf(peek, "%63s", magic) != 1) {
            std::fclose(peek);
            SetStatusMessage(Spark::Utf8String("Invalid scene file header."));
            return;
        }
        std::fclose(peek);

        if (std::strcmp(magic, SceneDocument::kMagic) == 0 || std::strcmp(magic, SceneDocument::kMagicV3) == 0) {
            LoadSceneFromPath(w, "scenes/arena.sparkscene");
            return;
        }

        const bool isV1 = std::strcmp(magic, "spark_scene_editor_v1") == 0;
        const bool isV2 = std::strcmp(magic, "spark_scene_editor_v2") == 0;
        if (!isV1 && !isV2) {
            SetStatusMessage(Spark::Utf8String("Invalid scene file header."));
            return;
        }
        std::FILE* f = std::fopen(path.CStr(), "r");
        if (f == nullptr) {
            SetStatusMessage(Spark::Utf8String("Could not reopen scene file."));
            return;
        }
        char magicAgain[64]{};
        if (std::fscanf(f, "%63s", magicAgain) != 1) {
            std::fclose(f);
            SetStatusMessage(Spark::Utf8String("Invalid scene file header."));
            return;
        }
        std::size_t n = 0;
        if (std::fscanf(f, "%zu", &n) != 1) {
            std::fclose(f);
            SetStatusMessage(Spark::Utf8String("Invalid scene file mesh count."));
            return;
        }
        {
            int c = 0;
            do {
                c = std::fgetc(f);
            } while (c != '\n' && c != EOF);
        }
        ClearPlaced(w);
        ClearUserLights(w);
        std::array<char, 512> lineBuf{};
        for (std::size_t k = 0; k < n; ++k) {
            if (std::fgets(lineBuf.data(), static_cast<int>(lineBuf.size()), f) == nullptr) {
                break;
            }
            char key[256]{};
            float sx = 1.0F;
            float sy = 1.0F;
            float sz = 1.0F;
            float tx = 0.0F;
            float ty = 0.0F;
            float tz = 0.0F;
            float qx = 0.0F;
            float qy = 0.0F;
            float qz = 0.0F;
            float qw = 1.0F;
            const int parsed = std::sscanf(
                    lineBuf.data(),
                    "%255s %f %f %f %f %f %f %f %f %f %f",
                    key,
                    &sx,
                    &sy,
                    &sz,
                    &tx,
                    &ty,
                    &tz,
                    &qx,
                    &qy,
                    &qz,
                    &qw);
            if (parsed < 11) {
                continue;
            }
            Spark::GameObject* go = w.CreateGameObject();
            go->GetName() = Spark::Utf8String("SceneEditorPlaced");
            Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({tx, ty, tz});
            tr->SetRotation({qx, qy, qz, qw});
            tr->SetScale({sx, sy, sz});

            if (std::strcmp(key, "builtin:unit_cube") == 0) {
                go->AddComponent<Spark::MeshComponent>(
                        unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.72F, 0.58F, 0.42F});
                if (Spark::MaterialComponent* m = go->AddComponent<Spark::MaterialComponent>()) {
                    m->SetMetallic(0.04F);
                    m->SetRoughness(0.55F);
                }
            } else {
                Spark::Utf8String full(SPARK_ASSETS_DIR);
                full.AppendUtf8("/");
                full.AppendUtf8(key);
                Spark::GltfAsset g{};
                if (!w.AwaitGltf(full.CStr(), g) || !g.mesh) {
                    w.DestroyGameObject(go);
                    continue;
                }
                Spark::GltfAssetBinder::BindRigidMesh(
                        *go, g, Spark::SceneMeshSlot::Custom, Spark::Vector3{1.0F, 1.0F, 1.0F}, full.CStr());
            }
            contentModel.TrackRoot(go);
            contentModel.TrackPlaced(go, Spark::Utf8String(key));
        }

        if (isV2) {
            std::size_t nl = 0;
            if (std::fscanf(f, "%zu", &nl) == 1) {
                int c2 = 0;
                do {
                    c2 = std::fgetc(f);
                } while (c2 != '\n' && c2 != EOF);
                for (std::size_t li = 0; li < nl; ++li) {
                    if (std::fgets(lineBuf.data(), static_cast<int>(lineBuf.size()), f) == nullptr) {
                        break;
                    }
                    float lx = 0.0F;
                    float ly = 0.0F;
                    float lz = 0.0F;
                    float cr = 1.0F;
                    float cg = 1.0F;
                    float cb = 1.0F;
                    float intens = 3.5F;
                    float rng = 20.0F;
                    if (std::sscanf(lineBuf.data(), "%f %f %f %f %f %f %f %f", &lx, &ly, &lz, &cr, &cg, &cb, &intens, &rng) == 8) {
                        (void)AddUserPointLightAt(w, {lx, ly, lz}, {cr, cg, cb}, intens, rng);
                    }
                }
            }
        }

        std::fclose(f);
        const std::string loaded = std::format(
                "Loaded {} meshes, {} lights from scene_editor/scene.txt",
                contentModel.GetPlacedObjects().GetSize(),
                contentModel.GetUserLights().GetSize());
        SetStatusMessage(Spark::Utf8String(loaded.c_str()));
    }

void SceneEditor3DDemo::ReloadSceneForPlayMode(Spark::GameWorld& w)
{
        UnloadEditorSceneContent(w);
        LoadSceneFromFile(w);
        if (sceneLoadInProgress && loadSession) {
            while (sceneLoadInProgress) {
                loadSession->Pump();
                if (loadedSceneId != Spark::kInvalidSceneInstanceId && loadSession->IsReady(loadedSceneId)) {
                    FinalizeAsyncSceneLoad(w);
                    sceneLoadInProgress = false;
                } else if (loadedSceneId != Spark::kInvalidSceneInstanceId && loadSession->HasFailed(loadedSceneId)) {
                    sceneLoadInProgress = false;
                    SetStatusMessage(Spark::Utf8String("Play mode failed (scene load error)."));
                    return;
                }
            }
        }
    }

void SceneEditor3DDemo::MaterialFilePath(char* out, const std::size_t outSz) noexcept
{
        if (out == nullptr || outSz == 0U) {
            return;
        }
        std::snprintf(out, outSz, "%s/materials/editor_selection.sparkmat", SPARK_BUILD_ASSETS_DIR);
}

void SceneEditor3DDemo::TrySaveSelectedMaterial(Spark::GameWorld& w)
{
        if (selectedObject == nullptr) {
            SetStatusMessage(Spark::Utf8String("Select an object with a MaterialComponent first."));
            return;
        }
        Spark::MaterialComponent* mat = selectedObject->GetComponent<Spark::MaterialComponent>();
        if (mat == nullptr) {
            SetStatusMessage(Spark::Utf8String("Selected object has no MaterialComponent."));
            return;
        }

        char path[512];
        MaterialFilePath(path, sizeof(path));
        constexpr const char* kAssetKey = "materials/editor_selection.sparkmat";

        Spark::MaterialAsset asset{};
        asset.name = Spark::Utf8String(kAssetKey);
        asset.CaptureFromMaterial(*mat);
        if (!w.SaveMaterialAsset(path, asset, SPARK_BUILD_ASSETS_DIR, &w.GetAssetCache())) {
            SetStatusMessage(Spark::Utf8String("Failed to save .sparkmat file."));
            return;
        }
        w.RegisterMaterial(asset, kAssetKey);
        mat->SetMaterialAsset(w, kAssetKey);
        SetStatusMessage(Spark::Utf8String("Saved materials/editor_selection.sparkmat (F10 to reload)."));
}

void SceneEditor3DDemo::TryLoadSelectedMaterial(Spark::GameWorld& w)
{
        if (selectedObject == nullptr) {
            SetStatusMessage(Spark::Utf8String("Select an object with a MaterialComponent first."));
            return;
        }
        Spark::MaterialComponent* mat = selectedObject->GetComponent<Spark::MaterialComponent>();
        if (mat == nullptr) {
            SetStatusMessage(Spark::Utf8String("Selected object has no MaterialComponent."));
            return;
        }

        constexpr const char* kAssetKey = "materials/editor_selection.sparkmat";

        (void)w.ReleaseAsset(Spark::CachedAssetKind::Material, kAssetKey);
        w.InvalidateAssetLoadState(kAssetKey, Spark::AssetLoadJobKind::Material);

        const Spark::AssetLoadOutcome<Spark::MaterialAsset> outcome = w.TryLoadMaterial(kAssetKey);
        if (!outcome.ok) {
            SetStatusMessage(Spark::Utf8String("Failed to load .sparkmat (save with F9 first)."));
            return;
        }
        w.RegisterMaterial(outcome.value, kAssetKey);
        mat->SetMaterialAsset(w, kAssetKey);
        mat->TryApplyMaterialAsset(w);
        SetStatusMessage(Spark::Utf8String("Loaded materials/editor_selection.sparkmat onto selection."));
}

void SceneEditor3DDemo::FocusCameraOnSelection() noexcept
{
        if (selectedObject == nullptr) {
            SetStatusMessage(Spark::Utf8String("Select an object in the viewport first."));
            return;
        }
        Spark::TransformComponent* tr = selectedObject->GetComponent<Spark::TransformComponent>();
        if (tr == nullptr) {
            return;
        }
        cameraController.FocusOn(tr->GetLocalTransform().translation);
        SetStatusMessage(Spark::Utf8String("Camera focused on selection."));
    }

void SceneEditor3DDemo::ResetEditorCamera() noexcept
{
        cameraController.ResetToDefault();
        SetStatusMessage(Spark::Utf8String("Camera reset to default view."));
    }


bool SceneEditor3DDemo::IsPointerInEditorViewport(const float cursorX, const int framebufferWidth) const noexcept
{
        return cursorX >= Spark::Ui::GetSceneEditorSidebarWidthPx() && cursorX < static_cast<float>(framebufferWidth);
    }

void SceneEditor3DDemo::SetupContextMenuCanvas(Spark::GameWorld& /*w*/)
{
        Spark::Ui::GetUiContextMenu().Close();
    }

void SceneEditor3DDemo::OpenSceneContextMenu(
        const float menuX,
        const float menuY,
        const Spark::Vector3& groundHit,
        Spark::GameObject* selection,
        Spark::GameWorld& world)
{
        ScenePlacementContext ctx = MakePlacementContext(world, groundHit, selection);
        Spark::Array<Spark::Utf8String> labels;
        placementActions.BuildMenu(labels, ctx);

        SceneEditor3DDemo* self = this;
        const Spark::Vector3 hit = groundHit;
        Spark::GameObject* selected = selection;
        Spark::Ui::GetUiContextMenu().Open(
                menuX,
                menuY,
                Spark::MoveTemp(labels),
                [self, hit, selected, &world](const int idx) {
                    ScenePlacementContext placementCtx = self->MakePlacementContext(world, hit, selected);
                    self->placementActions.Execute(static_cast<std::size_t>(idx), placementCtx);
                });
    }

ScenePlacementContext SceneEditor3DDemo::MakePlacementContext(
        Spark::GameWorld& world,
        const Spark::Vector3& groundHit,
        Spark::GameObject* selection) noexcept
{
        ScenePlacementContext ctx{
                world,
                *sceneManager,
                contentModel,
                instanceTracker,
                prefabCatalog,
                groundHit,
                selection,
                unitCubeAsset,
                &SceneEditor3DDemo::SyncLightGizmoEmissive,
                &SceneEditor3DDemo::SetStatusFromPlacement,
                this,
        };
        return ctx;
}

void SceneEditor3DDemo::SetStatusFromPlacement(const char* message, void* userData) noexcept
{
        auto* self = static_cast<SceneEditor3DDemo*>(userData);
        if (self != nullptr && message != nullptr) {
            self->SetStatusMessage(Spark::Utf8String(message));
        }
}

bool SceneEditor3DDemo::PlacementImportGltf(ScenePlacementContext& ctx)
{
        auto* self = static_cast<SceneEditor3DDemo*>(ctx.statusUserData);
        if (self == nullptr) {
            return false;
        }
        const GltfImportService::ImportResult importResult =
                self->gltfImportService.ImportFromFilePickerAndPlace(ctx.world, ctx);
        self->SetStatusMessage(importResult.message);
        return importResult.ok;
    }

bool SceneEditor3DDemo::PlacementDeleteSelected(ScenePlacementContext& ctx)
{
        auto* self = static_cast<SceneEditor3DDemo*>(ctx.statusUserData);
        if (self == nullptr || ctx.selected == nullptr) {
            return false;
        }
        self->RemoveEditorSelection(ctx.world, ctx.selected);
        self->SetStatusMessage(Spark::Utf8String("Removed selection."));
        return true;
    }

bool SceneEditor3DDemo::PlacementDeleteAvailable(const ScenePlacementContext& ctx)
{
        return ctx.selected != nullptr;
    }

bool SceneEditor3DDemo::PlacementSaveScene(ScenePlacementContext& ctx)
{
        auto* self = static_cast<SceneEditor3DDemo*>(ctx.statusUserData);
        if (self == nullptr) {
            return false;
        }
        self->SaveSceneToFile(ctx.world);
        return true;
    }

bool SceneEditor3DDemo::PlacementLoadScene(ScenePlacementContext& ctx)
{
        auto* self = static_cast<SceneEditor3DDemo*>(ctx.statusUserData);
        if (self == nullptr) {
            return false;
        }
        self->LoadSceneFromFile(ctx.world);
        return true;
    }

void SceneEditor3DDemo::ReloadSceneForPlayModeStatic(Spark::GameWorld& world, void* userData) noexcept
{
        auto* self = static_cast<SceneEditor3DDemo*>(userData);
        if (self != nullptr) {
            self->ReloadSceneForPlayMode(world);
        }
    }

void SceneEditor3DDemo::LoadSceneFromPath(Spark::GameWorld& w, const char* relativeScenePath)
{
        if (relativeScenePath == nullptr || relativeScenePath[0] == '\0') {
            SetStatusMessage(Spark::Utf8String("Invalid scene path."));
            return;
        }
        const Utf8String path = ScenePathResolver::ResolveReadablePath(relativeScenePath);
        if (!ScenePathResolver::FileExists(path.CStr())) {
            SetStatusMessage(Spark::Utf8String("Scene file not found."));
            return;
        }
        std::FILE* peek = std::fopen(path.CStr(), "r");
        if (peek == nullptr) {
            SetStatusMessage(Spark::Utf8String("Could not open scene file."));
            return;
        }
        char magic[64]{};
        if (std::fscanf(peek, "%63s", magic) != 1) {
            std::fclose(peek);
            SetStatusMessage(Spark::Utf8String("Invalid scene file header."));
            return;
        }
        std::fclose(peek);

        if (std::strcmp(magic, SceneDocument::kMagic) == 0 || std::strcmp(magic, SceneDocument::kMagicV3) == 0) {
            SceneDocument document;
            SceneDeserializer deserializer;
            if (!deserializer.ReadFromFile(path.CStr(), document)) {
                SetStatusMessage(Spark::Utf8String("Invalid spark scene file."));
                return;
            }
            if (!sceneManager) {
                SetStatusMessage(Spark::Utf8String("Scene manager not initialized."));
                return;
            }
            ClearPlaced(w);
            ClearUserLights(w);
            pendingLoadDocument = document;
            SceneLoadOptions options{};
            options.assetsRoot = ScenePathResolver::AssetsRoot();
            options.additive = true;
            loadedSceneId = sceneManager->BeginLoadSceneAsync(document, path.CStr(), options);
            if (loadedSceneId == Spark::kInvalidSceneInstanceId) {
                pendingLoadDocument = SceneDocument{};
                SetStatusMessage(Spark::Utf8String("Scene load failed to start."));
                return;
            }
            sceneLoadInProgress = true;
            selectedObject = nullptr;
            lightEditTarget = nullptr;
            if (sceneManager->IsSceneReady(loadedSceneId)) {
                FinalizeAsyncSceneLoad(w);
                sceneLoadInProgress = false;
            } else {
                SetStatusMessage(Spark::Utf8String("Loading scene (async assets)…"));
            }
            return;
        }

        SetStatusMessage(Spark::Utf8String("Unsupported scene format (use spark_scene_v4)."));
    }

void SceneEditor3DDemo::PlacePrefabFromAsset(Spark::GameWorld& w, const SceneEditorAssetEntry& entry)
{
        if (entry.kind != SceneEditorAssetKind::Prefab || !sceneManager) {
            SetStatusMessage(Spark::Utf8String("Select a prefab in the asset browser."));
            return;
        }
        const Utf8String path = ScenePathResolver::ResolveReadablePath(entry.relativePath.CStr());
        PrefabInstantiateOptions options{};
        options.assetsRoot = ScenePathResolver::AssetsRoot();
        options.position = {lastGroundHit.x, 0.0F, lastGroundHit.z};
        options.additive = true;
        options.pumpUntilReady = true;

        const PrefabInstantiateResult result = sceneManager->InstantiatePrefab(path.CStr(), options);
        if (!result.ready || result.rootObjects.IsEmpty()) {
            SetStatusMessage(Spark::Utf8String("Could not instantiate prefab."));
            return;
        }

        instanceTracker.Register(result.instanceId);
        Utf8String captureHint = entry.relativePath;
        for (std::size_t i = 0; i < result.rootObjects.GetSize(); ++i) {
            GameObject* root = result.rootObjects[i];
            if (root == nullptr) {
                continue;
            }
            contentModel.TrackRoot(root);
            contentModel.TrackPlaced(root, captureHint);
            selectedObject = root;
        }
        SetStatusMessage(Spark::Utf8String("Placed prefab from asset browser."));
    }

void SceneEditor3DDemo::OnAssetBrowserPlacePrefab(const SceneEditorAssetEntry& entry)
{
        if (playSession.IsActive() || editorWorld_ == nullptr) {
            return;
        }
        PlacePrefabFromAsset(*editorWorld_, entry);
    }

void SceneEditor3DDemo::OnAssetBrowserLoadScene(const SceneEditorAssetEntry& entry)
{
        if (playSession.IsActive() || editorWorld_ == nullptr) {
            return;
        }
        if (entry.kind != SceneEditorAssetKind::Scene) {
            SetStatusMessage(Spark::Utf8String("Select a scene in the asset browser."));
            return;
        }
        LoadSceneFromPath(*editorWorld_, entry.relativePath.CStr());
    }

void SceneEditor3DDemo::OnAssetBrowserImportGltf()
{
        if (playSession.IsActive() || editorWorld_ == nullptr) {
            return;
        }
        ScenePlacementContext placementCtx = MakePlacementContext(*editorWorld_, lastGroundHit, selectedObject);
        const GltfImportService::ImportResult importResult =
                gltfImportService.ImportFromFilePickerAndPlace(*editorWorld_, placementCtx);
        SetStatusMessage(importResult.message);
        if (importResult.ok) {
            assetCatalog.Refresh();
            assetBrowser.RefreshListFromCatalog();
        }
    }

void SceneEditor3DDemo::OnAssetBrowserRefreshCatalog()
{
        assetCatalog.Refresh();
        assetBrowser.RefreshListFromCatalog();
        SetStatusMessage(Spark::Utf8String("Asset list refreshed."));
    }

}  // namespace Spark
