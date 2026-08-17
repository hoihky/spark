#include "spark/demo/SceneEditor3DDemo.hpp"
#include "spark/demo/SceneEditor3DDemo_detail.hpp"
#include "spark/scene/GltfAssetBindings.hpp"

#include "spark/ecs/components/lighting/DirectionalLightComponent.hpp"
#include "spark/ecs/components/lighting/SpotLightComponent.hpp"
#include "spark/ui/runtime/UiContextMenu.hpp"
#include "spark/ui/runtime/UiScene.hpp"
#include "spark/scene/MeshRaycast.hpp"
#include "spark/scene/SceneRaycast.hpp"
#include "spark/scene/SceneSubmit.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"

namespace Spark {

void SceneEditor3DDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        selectedObject = nullptr;
        dragPlaced = nullptr;
        gizmoDragAxis = -1;
        orbitDragActive = false;
        rmbDragDistSq = 0.0F;
        selectionPulseTime = 0.0F;
        lightEditTarget = nullptr;
        statusMessage.Clear();
        roots.Clear();
        placed.Clear();
        placedRel.Clear();
        userLights.Clear();
        unitCubeAsset.Reset();
        groundAsset.Reset();
        loadedSceneId = Spark::kInvalidSceneInstanceId;
        pendingLoadDocument = SceneDocument{};
        sceneLoadInProgress = false;
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
        roots.PushBack(ground);

        Spark::GameObject* sun = w.CreateGameObject();
        sun->GetName() = Spark::Utf8String("SceneEditorSun");
        Spark::TransformComponent* str = sun->AddComponent<Spark::TransformComponent>();
        str->SetTranslation({24.0F, 38.0F, 18.0F});
        const Spark::Vector3 sunDir = Spark::Vector3{0.35F, 0.82F, 0.38F}.Normalized();
        str->SetRotation(Spark::Quaternion::FromShortestArc(Spark::Vector3::UnitZ, sunDir));
        sun->AddComponent<Spark::DirectionalLightComponent>(Spark::Vector3{1.0F, 0.97F, 0.92F}, 0.92F);
        roots.PushBack(sun);

        SetupContextMenuCanvas(w);

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("SceneEditorFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(480.0F, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*fpsText);
        fpsText->SetText(Spark::Utf8String("Scene editor — RMB menu · drag RMB look · Alt+LMB orbit · F1 fly"));
        roots.PushBack(fpsHudObject);

        sceneManager = Spark::MakeUnique<Spark::SceneManager>(w);

        context.GetInput().SetCursorCaptured(false);
        camera.position = {8.0F, 6.5F, 14.0F};
        cameraOrbitPivot = {0.0F, 0.0F, 0.0F};
        camera.SnapLookAt(cameraOrbitPivot);
        {
            const Spark::Vector3 off{
                    camera.position.x - cameraOrbitPivot.x,
                    camera.position.y - cameraOrbitPivot.y,
                    camera.position.z - cameraOrbitPivot.z};
            cameraOrbitDistance = std::max(3.0F, off.Length());
        }
    }

void SceneEditor3DDemo::Unload(Spark::GameWorld& w)
{
        Spark::Ui::GetUiContextMenu().Close();
        if (sceneManager && loadedSceneId != Spark::kInvalidSceneInstanceId) {
            sceneManager->UnloadScene(loadedSceneId);
            loadedSceneId = Spark::kInvalidSceneInstanceId;
        }
        sceneLoadInProgress = false;
        pendingLoadDocument = SceneDocument{};
        sceneManager.Reset();
        ClearPlaced(w);
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        placed.Clear();
        placedRel.Clear();
        userLights.Clear();
        lightEditTarget = nullptr;
        statusMessage.Clear();
        orbitDragActive = false;
        selectedObject = nullptr;
        gizmoDragAxis = -1;
        fpsHudObject = nullptr;
        fpsText = nullptr;
        unitCubeAsset.Reset();
        groundAsset.Reset();
    }

void SceneEditor3DDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world)
{
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

        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsCursorCaptured()) {
            if (timing.frameIndex > 0) {
                camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
            }
            camera.ProcessMovement(in, timing.deltaTimeSeconds);
            const float scroll = in.GetScrollDeltaY();
            if (std::fabs(scroll) > 1.0e-4F) {
                camera.position += camera.Forward() * (scroll * 0.65F);
            }
        } else if (!UiConsumesGamePointer()) {
            const bool rmbDown = in.IsMouseButtonDown(1);
            const bool mmbDown = in.IsMouseButtonDown(2);
            const bool cameraNavActive = inViewport || rmbDown || mmbDown || orbitDragActive;

            if (in.IsMouseButtonPressedThisFrame(1) && inViewport) {
                rmbDragDistSq = 0.0F;
            }
            if (rmbDown && inViewport && timing.frameIndex > 0) {
                const float mdx = in.GetMouseDeltaX();
                const float mdy = in.GetMouseDeltaY();
                rmbDragDistSq += mdx * mdx + mdy * mdy;
                camera.AddLook(mdx, mdy);
            }

            if (mmbDown && inViewport && timing.frameIndex > 0) {
                const float panScale = 0.014F * std::max(1.0F, cameraOrbitDistance * 0.08F);
                PanFlyCamera(camera, in.GetMouseDeltaX(), in.GetMouseDeltaY(), panScale);
            }

            if (inViewport && std::fabs(in.GetScrollDeltaY()) > 1.0e-4F) {
                camera.position += camera.Forward() * (in.GetScrollDeltaY() * 0.65F);
            }

            if (cameraNavActive) {
                camera.ProcessMovement(in, timing.deltaTimeSeconds);
            }

            if (in.IsMouseButtonReleasedThisFrame(0)) {
                orbitDragActive = false;
            }
        }

        if (!in.IsCursorCaptured() && !UiConsumesGamePointer()) {
            const Spark::Matrix4 view = camera.ViewMatrix();
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
                gizmoDragAxis = -1;
            }

            const bool haveGround = haveRay && RayIntersectPlaneY(ro, rd, 0.0F, groundHit);
            const bool altHeld = in.IsKeyDown(GLFW_KEY_LEFT_ALT) || in.IsKeyDown(GLFW_KEY_RIGHT_ALT);

            if (in.IsMouseButtonReleasedThisFrame(1) && inViewport && rmbDragDistSq < 64.0F && haveGround) {
                OpenSceneContextMenu(mx, my, groundHit, selectedObject, world);
            }

            if (orbitDragActive && in.IsMouseButtonDown(0) && timing.frameIndex > 0) {
                OrbitFlyCameraAroundPivot(
                        camera,
                        cameraOrbitPivot,
                        cameraOrbitDistance,
                        in.GetMouseDeltaX(),
                        in.GetMouseDeltaY());
            } else if (haveRay && in.IsMouseButtonPressedThisFrame(0)) {
                bool handledPress = false;
                if (altHeld && inViewport) {
                    if (selectedObject != nullptr) {
                        Spark::TransformComponent* selTr = selectedObject->GetComponent<Spark::TransformComponent>();
                        if (selTr != nullptr) {
                            cameraOrbitPivot = selTr->GetLocalTransform().translation;
                        }
                    }
                    const Spark::Vector3 off{
                            camera.position.x - cameraOrbitPivot.x,
                            camera.position.y - cameraOrbitPivot.y,
                            camera.position.z - cameraOrbitPivot.z};
                    cameraOrbitDistance = std::max(1.5F, off.Length());
                    orbitDragActive = true;
                    handledPress = true;
                }
                if (!handledPress && selectedObject != nullptr) {
                    Spark::TransformComponent* selTr = selectedObject->GetComponent<Spark::TransformComponent>();
                    if (selTr != nullptr) {
                        const Spark::Vector3 pivot = selTr->GetLocalTransform().translation;
                        const float ext = SelectionGizmoExtent(selectedObject);
                        int axis = -1;
                        if (TryPickTranslateGizmo(ro, rd, pivot, ext, axis)) {
                            gizmoDragAxis = axis;
                            gizmoDragStartTranslation = pivot;
                            dragPlaced = selectedObject;
                            (void)ClosestRayLineParameter(
                                    ro, rd, pivot, SceneEditorGizmoAxisDir(axis), gizmoDragStartLineS);
                            handledPress = true;
                            SetStatusMessage(Spark::Utf8String("Drag the colored axis arrow to move along X, Y, or Z."));
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

            if (!orbitDragActive && haveRay && in.IsMouseButtonDown(0) && dragPlaced != nullptr && gizmoDragAxis >= 0) {
                float lineS = 0.0F;
                const Spark::Vector3 axis = SceneEditorGizmoAxisDir(gizmoDragAxis);
                if (ClosestRayLineParameter(ro, rd, gizmoDragStartTranslation, axis, lineS)) {
                    const float ds = lineS - gizmoDragStartLineS;
                    Spark::TransformComponent* dtr = dragPlaced->GetComponent<Spark::TransformComponent>();
                    if (dtr != nullptr) {
                        const Spark::Vector3 t{
                                gizmoDragStartTranslation.x + axis.x * ds,
                                gizmoDragStartTranslation.y + axis.y * ds,
                                gizmoDragStartTranslation.z + axis.z * ds};
                        dtr->SetTranslation(t);
                    }
                }
            } else if (!orbitDragActive && haveRay && in.IsMouseButtonDown(0) && dragPlaced != nullptr && gizmoDragAxis < 0) {
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

        if (fpsText != nullptr) {
            const float dt = timing.deltaTimeSeconds;
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            std::string hud = std::format(
                    "Scene editor — {:.0f} FPS · {} meshes · {} lights · RMB menu · LMB select · Alt+LMB orbit",
                    static_cast<double>(fpsSmoothed),
                    static_cast<int>(placed.GetSize()),
                    static_cast<int>(userLights.GetSize()));
            if (!statusMessage.IsEmpty()) {
                hud += " · ";
                hud += statusMessage.CStr();
            }
            fpsText->SetText(Spark::Utf8String(hud.c_str()));
            fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        }
    }

void SceneEditor3DDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 400.0F);
        const Spark::Matrix4 view = camera.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = camera.position;
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

        auto findOrAddTexture = [&params](const Spark::SharedPtr<Spark::Texture2D>& tex) -> std::int32_t {
            if (!tex) {
                return -1;
            }
            for (std::size_t i = 0; i < params.sceneTextures.GetSize(); ++i) {
                if (params.sceneTextures[i].Get() == tex.Get()) {
                    return static_cast<std::int32_t>(i);
                }
            }
            if (params.sceneTextures.GetSize() >= Spark::SceneRenderParams::MaxSceneTextures) {
                return -1;
            }
            params.sceneTextures.PushBack(tex);
            return static_cast<std::int32_t>(params.sceneTextures.GetSize() - 1U);
        };

        Spark::Array<Spark::SceneDrawItem> drawList;
        drawList.Reserve(32);
        scene.ForEachDrawable([&](Spark::GameObject* obj, const Spark::MeshComponent& mc,
                                     const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            Spark::SceneDrawItem item{};
            item.model = world;
            item.mesh = mc.GetSlot();
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom) {
                item.customMesh = mc.GetMesh();
            }
            Spark::Vector3 alb = mc.GetAlbedo();
            item.textureLayer = -1;
            if (mat != nullptr) {
                ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                if (mat->GetBaseColorTexture()) {
                    const Spark::Vector3& t = mat->GetTint();
                    alb = {alb.x * t.x, alb.y * t.y, alb.z * t.z};
                    item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
                }
            }
            if (obj != nullptr && obj == selectedObject) {
                const float pulse = 0.82F + 0.18F * std::sin(selectionPulseTime * 6.8F);
                const Spark::Vector3 rim{0.22F, 0.78F, 1.0F};
                item.emissiveColor = {
                        std::min(1.0F, item.emissiveColor.x + rim.x * 0.55F),
                        std::min(1.0F, item.emissiveColor.y + rim.y * 0.55F),
                        std::min(1.0F, item.emissiveColor.z + rim.z * 0.55F)};
                item.emissiveIntensity = item.emissiveIntensity + 2.5F * pulse;
                item.roughness = std::max(0.06F, item.roughness * 0.55F);
                alb = {std::min(1.0F, alb.x * 1.06F + 0.03F), std::min(1.0F, alb.y * 1.04F + 0.05F),
                        std::min(1.0F, alb.z * 1.12F + 0.06F)};
            }
            item.albedo = alb;
            drawList.PushBack(item);
        });

        StableSortDrawItems(drawList);
        PartitionSortedDrawItemsIntoSceneParams(drawList, params, camera.position);

        if (selectedObject != nullptr) {
            Spark::TransformComponent* selTr = selectedObject->GetComponent<Spark::TransformComponent>();
            if (selTr != nullptr) {
                const Spark::Vector3 pivot = selTr->GetLocalTransform().translation;
                const float ext = SelectionGizmoExtent(selectedObject);
                Spark::Array<Spark::SceneDrawItem> gizmoDraws;
                gizmoDraws.Reserve(6);
                AppendTranslateGizmoDraws(pivot, ext, gizmoDragAxis, gizmoDraws);
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

        context.SetSceneRenderParams(params);
    }

void SceneEditor3DDemo::SceneFilePath(char* out, std::size_t outSz) noexcept
{
        std::snprintf(out, outSz, "%s/scene_editor/scene.txt", SPARK_BUILD_ASSETS_DIR);
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

void SceneEditor3DDemo::ClearPlaced(Spark::GameWorld& w)
{
        if (sceneManager && loadedSceneId != Spark::kInvalidSceneInstanceId) {
            sceneManager->UnloadScene(loadedSceneId);
            loadedSceneId = Spark::kInvalidSceneInstanceId;
            sceneLoadInProgress = false;
            pendingLoadDocument = SceneDocument{};
            selectedObject = nullptr;
            dragPlaced = nullptr;
            gizmoDragAxis = -1;
            lightEditTarget = nullptr;
            placed.Clear();
            placedRel.Clear();
            userLights.Clear();
            return;
        }
        if (selectedObject != nullptr) {
            for (std::size_t i = 0; i < placed.GetSize(); ++i) {
                if (placed[i] == selectedObject) {
                    selectedObject = nullptr;
                    break;
                }
            }
        }
        dragPlaced = nullptr;
        gizmoDragAxis = -1;
        for (std::size_t i = 0; i < placed.GetSize(); ++i) {
            if (placed[i] != nullptr) {
                w.DestroyGameObject(placed[i]);
            }
        }
        placed.Clear();
        placedRel.Clear();
    }

void SceneEditor3DDemo::RemoveEditorSelection(Spark::GameWorld& w, Spark::GameObject* go) noexcept
{
        if (go == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < placed.GetSize(); ++i) {
            if (placed[i] == go) {
                w.DestroyGameObject(go);
                placed.RemoveAt(i);
                if (i < placedRel.GetSize()) {
                    placedRel.RemoveAt(i);
                }
                if (selectedObject == go) {
                    selectedObject = nullptr;
                }
                if (dragPlaced == go) {
                    dragPlaced = nullptr;
                }
                if (lightEditTarget == go) {
                    lightEditTarget = nullptr;
                }
                gizmoDragAxis = -1;
                return;
            }
        }
        for (std::size_t j = 0; j < userLights.GetSize(); ++j) {
            if (userLights[j] == go) {
                w.DestroyGameObject(go);
                userLights.RemoveAt(j);
                if (selectedObject == go) {
                    selectedObject = nullptr;
                }
                if (dragPlaced == go) {
                    dragPlaced = nullptr;
                }
                if (lightEditTarget == go) {
                    lightEditTarget = nullptr;
                }
                gizmoDragAxis = -1;
                return;
            }
        }
    }

void SceneEditor3DDemo::ClearUserLights(Spark::GameWorld& w)
{
        if (selectedObject != nullptr) {
            for (std::size_t i = 0; i < userLights.GetSize(); ++i) {
                if (userLights[i] == selectedObject) {
                    selectedObject = nullptr;
                    break;
                }
            }
        }
        dragPlaced = nullptr;
        gizmoDragAxis = -1;
        lightEditTarget = nullptr;
        for (std::size_t i = 0; i < userLights.GetSize(); ++i) {
            if (userLights[i] != nullptr) {
                w.DestroyGameObject(userLights[i]);
            }
        }
        userLights.Clear();
    }

[[nodiscard]] bool SceneEditor3DDemo::IsUserLight(Spark::GameObject* go) const noexcept
{
        if (go == nullptr) {
            return false;
        }
        for (std::size_t i = 0; i < userLights.GetSize(); ++i) {
            if (userLights[i] == go) {
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

void SceneEditor3DDemo::LightPresetParams(
        const int preset, Spark::Vector3& outColor, float& outIntensity, float& outRange) noexcept
{
        switch (preset) {
        case 1:
            outColor = {0.72F, 0.88F, 1.0F};
            outIntensity = 3.6F;
            outRange = 30.0F;
            break;
        case 2:
            outColor = {0.95F, 0.55F, 1.0F};
            outIntensity = 3.9F;
            outRange = 20.0F;
            break;
        default:
            outColor = {1.0F, 0.88F, 0.68F};
            outIntensity = 4.4F;
            outRange = 24.0F;
            break;
        }
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
        roots.PushBack(go);
        userLights.PushBack(go);
        return go;
    }

[[nodiscard]] bool SceneEditor3DDemo::TrySpawnUserPointLight(
        Spark::GameWorld& w, const Spark::Vector3& groundHit, const int presetIndex) noexcept
{
        static constexpr std::size_t kMaxUserLights = 7;
        if (userLights.GetSize() >= kMaxUserLights) {
            return false;
        }
        Spark::Vector3 color{};
        float intensity = 4.0F;
        float range = 22.0F;
        LightPresetParams(presetIndex, color, intensity, range);
        constexpr float kLiftY = 2.75F;
        Spark::GameObject* spawned =
                AddUserPointLightAt(w, {groundHit.x, kLiftY, groundHit.z}, color, intensity, range);
        lightEditTarget = spawned;
        return spawned != nullptr;
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
            for (std::size_t i = placed.GetSize(); i > 0U; --i) {
                if (placed[i - 1U] == hit.object) {
                    bestT = hit.distance;
                    bestGo = hit.object;
                    bestHit = hit.pointWorld;
                    break;
                }
            }
        }
    }
    if (pickLights) {
        for (std::size_t j = userLights.GetSize(); j > 0U; --j) {
            Spark::GameObject* go = userLights[j - 1U];
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

[[nodiscard]] const char* SceneEditor3DDemo::PresetRelPath(int idx) noexcept
{
        static constexpr const char* kPaths[] = {
                "models/DamagedHelmet.glb",
                "models/SheenChair.glb",
                "builtin:unit_cube",
        };
        if (idx < 0 || idx >= 3) {
            return kPaths[0];
        }
        return kPaths[idx];
    }

[[nodiscard]] bool SceneEditor3DDemo::TryPlaceAtPreset(
        Spark::GameWorld& w, const Spark::Vector3& hitXZ, const int presetIndex)
{
        const int idx = std::clamp(presetIndex, 0, 2);
        const char* rel = PresetRelPath(idx);
        Spark::GameObject* go = w.CreateGameObject();
        go->GetName() = Spark::Utf8String("SceneEditorPlaced");
        Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
        Spark::Quaternion rot = Spark::Quaternion::Identity;
        float uniformScale = 1.0F;

        if (std::strcmp(rel, "builtin:unit_cube") == 0) {
            uniformScale = 0.85F;
            tr->SetTranslation({hitXZ.x, uniformScale, hitXZ.z});
            tr->SetUniformScale(uniformScale);
            go->AddComponent<Spark::MeshComponent>(
                    unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.72F, 0.58F, 0.42F});
            if (Spark::MaterialComponent* m = go->AddComponent<Spark::MaterialComponent>()) {
                m->SetMetallic(0.04F);
                m->SetRoughness(0.55F);
            }
        } else {
            Spark::Utf8String full(SPARK_ASSETS_DIR);
            full.AppendUtf8("/");
            full.AppendUtf8(rel);
            Spark::GltfAsset g{};
            if (!w.AwaitGltf(full.CStr(), g) || !g.mesh) {
                w.DestroyGameObject(go);
                return false;
            }
            Spark::Vector3 bmin{};
            Spark::Vector3 bmax{};
            uniformScale = 1.8F;
            if (g.mesh->TryComputeAxisAlignedBounds(bmin, bmax)) {
                const float dx = bmax.x - bmin.x;
                const float dy = bmax.y - bmin.y;
                const float dz = bmax.z - bmin.z;
                const float maxExt = std::max({dx, dy, dz});
                if (maxExt > 1.0e-4F) {
                    uniformScale = 2.2F / maxExt;
                }
            }
            if (std::strstr(rel, "DamagedHelmet") != nullptr) {
                rot = Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitY, Spark::Pi);
            }
            constexpr float kGroundClearance = 0.08F;
            const float yOnGround = -bmin.y * uniformScale + kGroundClearance;
            tr->SetUniformScale(uniformScale);
            tr->SetTranslation({hitXZ.x, yOnGround, hitXZ.z});
            tr->SetRotation(rot);
            Spark::GltfAssetBinder::BindRigidMesh(
                    *go, g, Spark::SceneMeshSlot::Custom, Spark::Vector3{1.0F, 1.0F, 1.0F});
        }

        roots.PushBack(go);
        placed.PushBack(go);
        placedRel.PushBack(Spark::Utf8String(rel));
        return true;
    }

void SceneEditor3DDemo::SaveSceneToFile(Spark::GameWorld& w)
{
        char path[512]{};
        SceneFilePath(path, sizeof(path));

        struct CaptureCtx {
            SceneEditor3DDemo* self;
        } captureCtx{this};

        SceneCaptureContext ctx{};
        ctx.meshAssetUserData = &captureCtx;
        ctx.textureUserData = &captureCtx;
        ctx.resolveMeshAssetPath = [](const GameObject& owner, void* userData) -> Utf8String {
            auto* c = static_cast<CaptureCtx*>(userData);
            if (c == nullptr || c->self == nullptr) {
                return Utf8String{};
            }
            SceneEditor3DDemo* self = c->self;
            for (std::size_t i = 0; i < self->placed.GetSize(); ++i) {
                if (self->placed[i] == &owner) {
                    return self->placedRel[i];
                }
            }
            return Utf8String{};
        };
        ctx.resolveTexturePath = [](const GameObject& owner, void* userData) -> Utf8String {
            auto* c = static_cast<CaptureCtx*>(userData);
            if (c == nullptr || c->self == nullptr) {
                return Utf8String{};
            }
            const MaterialComponent* mat = owner.GetComponent<MaterialComponent>();
            if (mat == nullptr || !mat->GetBaseColorTexture()) {
                return Utf8String{};
            }
            SceneEditor3DDemo* self = c->self;
            for (std::size_t i = 0; i < self->placed.GetSize(); ++i) {
                if (self->placed[i] == &owner) {
                    const Utf8String& rel = self->placedRel[i];
                    if (rel.IsEmpty() || std::strcmp(rel.CStr(), "builtin:unit_cube") == 0) {
                        return Utf8String{};
                    }
                    return rel;
                }
            }
            return Utf8String{};
        };

        const auto includeEntity = [this](const GameObject* go) -> bool {
            if (go == nullptr) {
                return false;
            }
            for (std::size_t i = 0; i < placed.GetSize(); ++i) {
                if (placed[i] == go) {
                    return true;
                }
            }
            for (std::size_t i = 0; i < userLights.GetSize(); ++i) {
                if (userLights[i] == go) {
                    return true;
                }
            }
            return false;
        };

        SceneSerializer serializer;
        const SceneDocument document = serializer.Capture(w, ctx, includeEntity);
        if (!serializer.WriteToFile(document, path)) {
            SetStatusMessage(Spark::Utf8String("Save failed (could not open file)."));
            return;
        }
        const std::string saved = std::format(
                "Saved {} entities (spark_scene_v4) → scene_editor/scene.txt",
                document.entities.GetSize());
        SetStatusMessage(Spark::Utf8String(saved.c_str()));
    }

void SceneEditor3DDemo::SortObjectsById(Spark::Array<Spark::GameObject*>& objects) noexcept
{
        for (std::size_t i = 1; i < objects.GetSize(); ++i) {
            Spark::GameObject* key = objects[i];
            const std::uint64_t keyId = key != nullptr ? key->GetId() : 0;
            std::size_t j = i;
            while (j > 0) {
                Spark::GameObject* prev = objects[j - 1];
                const std::uint64_t prevId = prev != nullptr ? prev->GetId() : 0;
                if (prevId <= keyId) {
                    break;
                }
                objects[j] = objects[j - 1];
                --j;
            }
            objects[j] = key;
        }
}

void SceneEditor3DDemo::FinalizeAsyncSceneLoad(Spark::GameWorld& w)
{
        (void)w;
        if (loadedSceneId == Spark::kInvalidSceneInstanceId) {
            return;
        }
        Spark::Array<Spark::GameObject*> instanceObjects;
        w.ForEachGameObject([&](Spark::GameObject* object) {
            if (object != nullptr && object->GetSceneInstanceId() == loadedSceneId) {
                instanceObjects.PushBack(object);
            }
        });
        SortObjectsById(instanceObjects);

        for (std::size_t ei = 0; ei < pendingLoadDocument.entities.GetSize() && ei < instanceObjects.GetSize(); ++ei) {
            const EntityRecord& entity = pendingLoadDocument.entities[ei];
            Spark::GameObject* object = instanceObjects[ei];
            if (object == nullptr) {
                continue;
            }
            bool hasMesh = false;
            bool hasLight = false;
            Spark::Utf8String meshAsset;
            for (std::size_t ci = 0; ci < entity.components.GetSize(); ++ci) {
                const ComponentRecord& component = entity.components[ci];
                if (component.kind == Spark::Utf8String("mesh")) {
                    hasMesh = true;
                    char slotTag[32]{};
                    char asset[384]{};
                    float ar = 1.0F;
                    float ag = 1.0F;
                    float ab = 1.0F;
                    if (std::sscanf(
                                component.payload.CStr(),
                                "%31s \"%383[^\"]\" %f %f %f",
                                slotTag,
                                asset,
                                &ar,
                                &ag,
                                &ab)
                        >= 2) {
                        meshAsset = Spark::Utf8String(asset);
                    }
                } else if (component.kind == Spark::Utf8String("point_light")
                           || component.kind == Spark::Utf8String("spot_light")) {
                    hasLight = true;
                }
            }
            if (hasLight) {
                roots.PushBack(object);
                userLights.PushBack(object);
                if (object->GetComponent<Spark::MeshComponent>() == nullptr && unitCubeAsset) {
                    object->AddComponent<Spark::MeshComponent>(
                            unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{1.0F, 1.0F, 1.0F});
                    if (Spark::MaterialComponent* m = object->AddComponent<Spark::MaterialComponent>()) {
                        m->SetMetallic(0.12F);
                        m->SetRoughness(0.35F);
                        if (Spark::PointLightComponent* pl = object->GetComponent<Spark::PointLightComponent>()) {
                            m->SetEmissive(pl->GetColor(), 7.5F);
                        } else if (Spark::SpotLightComponent* sl = object->GetComponent<Spark::SpotLightComponent>()) {
                            m->SetEmissive(sl->GetColor(), 7.5F);
                        }
                    }
                    SyncLightGizmoEmissive(object);
                }
            } else if (hasMesh) {
                roots.PushBack(object);
                placed.PushBack(object);
                placedRel.PushBack(meshAsset);
            }
        }
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
        char path[512]{};
        SceneFilePath(path, sizeof(path));
        std::FILE* peek = std::fopen(path, "r");
        if (peek == nullptr) {
            SetStatusMessage(Spark::Utf8String("No save file yet (scene_editor/scene.txt)."));
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
            if (!deserializer.ReadFromFile(path, document)) {
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
            Spark::SceneLoadOptions options{};
            options.assetsRoot = SPARK_ASSETS_DIR;
            options.additive = true;
            loadedSceneId = sceneManager->BeginLoadSceneAsync(document, path, options);
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

        const bool isV1 = std::strcmp(magic, "spark_scene_editor_v1") == 0;
        const bool isV2 = std::strcmp(magic, "spark_scene_editor_v2") == 0;
        if (!isV1 && !isV2) {
            SetStatusMessage(Spark::Utf8String("Invalid scene file header."));
            return;
        }
        std::FILE* f = std::fopen(path, "r");
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
                        *go, g, Spark::SceneMeshSlot::Custom, Spark::Vector3{1.0F, 1.0F, 1.0F});
            }
            roots.PushBack(go);
            placed.PushBack(go);
            placedRel.PushBack(Spark::Utf8String(key));
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
                placed.GetSize(),
                userLights.GetSize());
        SetStatusMessage(Spark::Utf8String(loaded.c_str()));
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
        cameraOrbitPivot = tr->GetLocalTransform().translation;
        const Spark::Vector3 off{
                camera.position.x - cameraOrbitPivot.x,
                camera.position.y - cameraOrbitPivot.y,
                camera.position.z - cameraOrbitPivot.z};
        cameraOrbitDistance = std::max(3.0F, off.Length());
        camera.SnapLookAt(cameraOrbitPivot);
        SetStatusMessage(Spark::Utf8String("Camera focused on selection."));
    }

void SceneEditor3DDemo::ResetEditorCamera() noexcept
{
        camera.position = {8.0F, 6.5F, 14.0F};
        cameraOrbitPivot = {0.0F, 0.0F, 0.0F};
        camera.SnapLookAt(cameraOrbitPivot);
        const Spark::Vector3 off{
                camera.position.x - cameraOrbitPivot.x,
                camera.position.y - cameraOrbitPivot.y,
                camera.position.z - cameraOrbitPivot.z};
        cameraOrbitDistance = std::max(3.0F, off.Length());
        SetStatusMessage(Spark::Utf8String("Camera reset to default view."));
    }


bool SceneEditor3DDemo::IsPointerInEditorViewport(const float cursorX, const int framebufferWidth) const noexcept
{
        return cursorX >= 0.0F && cursorX < static_cast<float>(framebufferWidth);
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
        Spark::Array<Spark::Utf8String> labels;
        Spark::Array<SceneEditorMenuAction> actions;
        auto pushItem = [&](const char* label, const SceneEditorMenuAction action) {
            labels.PushBack(Spark::Utf8String(label));
            actions.PushBack(action);
        };
        pushItem("Mesh — DamagedHelmet.glb", SceneEditorMenuAction::MeshDamagedHelmet);
        pushItem("Mesh — SheenChair.glb", SceneEditorMenuAction::MeshSheenChair);
        pushItem("Mesh — Unit cube (builtin)", SceneEditorMenuAction::MeshUnitCube);
        pushItem("Light — Warm tungsten", SceneEditorMenuAction::LightWarm);
        pushItem("Light — Cool daylight", SceneEditorMenuAction::LightCool);
        pushItem("Light — Soft magenta accent", SceneEditorMenuAction::LightMagenta);
        if (selection != nullptr) {
            pushItem("Delete selected", SceneEditorMenuAction::DeleteSelected);
        }
        pushItem("Save scene", SceneEditorMenuAction::SaveScene);
        pushItem("Load scene", SceneEditorMenuAction::LoadScene);

        SceneEditor3DDemo* self = this;
        const Spark::Vector3 spawnPos = groundHit;
        Spark::GameObject* pickTarget = selection;
        Spark::Ui::GetUiContextMenu().Open(
                menuX,
                menuY,
                Spark::MoveTemp(labels),
                [self, spawnPos, pickTarget, actions = Spark::MoveTemp(actions), &world](const int idx) {
                    if (idx < 0 || static_cast<std::size_t>(idx) >= actions.GetSize()) {
                        return;
                    }
                    switch (actions[static_cast<std::size_t>(idx)]) {
                    case SceneEditorMenuAction::MeshDamagedHelmet:
                        if (self->TryPlaceAtPreset(world, spawnPos, 0)) {
                            self->SetStatusMessage(Spark::Utf8String("Placed DamagedHelmet.glb."));
                        } else {
                            self->SetStatusMessage(Spark::Utf8String("Could not load DamagedHelmet.glb."));
                        }
                        break;
                    case SceneEditorMenuAction::MeshSheenChair:
                        if (self->TryPlaceAtPreset(world, spawnPos, 1)) {
                            self->SetStatusMessage(Spark::Utf8String("Placed SheenChair.glb."));
                        } else {
                            self->SetStatusMessage(Spark::Utf8String("Could not load SheenChair.glb."));
                        }
                        break;
                    case SceneEditorMenuAction::MeshUnitCube:
                        if (self->TryPlaceAtPreset(world, spawnPos, 2)) {
                            self->SetStatusMessage(Spark::Utf8String("Placed unit cube."));
                        } else {
                            self->SetStatusMessage(Spark::Utf8String("Could not place unit cube."));
                        }
                        break;
                    case SceneEditorMenuAction::LightWarm:
                        if (self->TrySpawnUserPointLight(world, spawnPos, 0)) {
                            self->SetStatusMessage(Spark::Utf8String("Placed warm tungsten light."));
                        } else {
                            self->SetStatusMessage(Spark::Utf8String("Too many lights (max 7 user lights)."));
                        }
                        break;
                    case SceneEditorMenuAction::LightCool:
                        if (self->TrySpawnUserPointLight(world, spawnPos, 1)) {
                            self->SetStatusMessage(Spark::Utf8String("Placed cool daylight light."));
                        } else {
                            self->SetStatusMessage(Spark::Utf8String("Too many lights (max 7 user lights)."));
                        }
                        break;
                    case SceneEditorMenuAction::LightMagenta:
                        if (self->TrySpawnUserPointLight(world, spawnPos, 2)) {
                            self->SetStatusMessage(Spark::Utf8String("Placed soft magenta light."));
                        } else {
                            self->SetStatusMessage(Spark::Utf8String("Too many lights (max 7 user lights)."));
                        }
                        break;
                    case SceneEditorMenuAction::DeleteSelected:
                        if (pickTarget != nullptr) {
                            self->RemoveEditorSelection(world, pickTarget);
                            self->SetStatusMessage(Spark::Utf8String("Removed selection."));
                        }
                        break;
                    case SceneEditorMenuAction::SaveScene:
                        self->SaveSceneToFile(world);
                        break;
                    case SceneEditorMenuAction::LoadScene:
                        self->LoadSceneFromFile(world);
                        break;
                    }
                });
    }

}  // namespace Spark
