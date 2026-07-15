#include "spark/demo/TerrainDemo.hpp"
#include "spark/demo/DemoAssetLoad.hpp"

namespace Spark {

void TerrainDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        terrainObject = nullptr;
        terrainComp = nullptr;
        editCursorObject = nullptr;
        editCursorTransform = nullptr;
        editCursorMaterial = nullptr;
        markerObject = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;

        groundTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("TerrainGround"));
        if (!DemoAssets::TryLoadTerrainDemoSoilTexture(*groundTex)) {
            *groundTex = Spark::Texture2D::CreateVariedSoilGroundPattern(1024, 1024);
        }
        w.RegisterTexture(groundTex, "spark/terrain/ground");

        Spark::TerrainGeneratorSettings ts{};
        ts.subdivX = 288;
        ts.subdivZ = 288;
        ts.halfExtentX = 220.0F;
        ts.halfExtentZ = 220.0F;
        ts.worldUnitsPerTextureRepeat = DemoAssets::ProceduralTextureSpanWorldUnits(ts.halfExtentX);
        ts.heightScale = 26.0F;
        ts.noiseScale = 0.0135F;
        ts.octaves = 6;
        ts.persistence = 0.5F;
        ts.lacunarity = 2.05F;
        ts.seed = 0xC047ACEEu;

        terrainObject = w.CreateGameObject();
        terrainObject->GetName() = Spark::Utf8String("ProceduralTerrain");
        terrainObject->AddComponent<Spark::TransformComponent>();
        terrainObject->AddComponent<Spark::TerrainComponent>(ts);
        terrainComp = terrainObject->GetComponent<Spark::TerrainComponent>();
        if (Spark::MaterialComponent* m = terrainObject->AddComponent<Spark::MaterialComponent>(
                    groundTex, Spark::Vector3::One)) {
            m->SetMetallic(0.02F);
            m->SetRoughness(0.94F);
        }
        roots.PushBack(terrainObject);

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("TerrainMarkerCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        w.RegisterMesh(unitCubeAsset, "spark/terrain/marker_cube");
        markerObject = w.CreateGameObject();
        markerObject->GetName() = Spark::Utf8String("TerrainMarker");
        {
            Spark::TransformComponent* tr = markerObject->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({55.0F, 9.0F, 72.0F});
            tr->SetUniformScale(0.9F);
        }
        markerObject->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.85F, 0.38F, 0.22F});
        if (Spark::MaterialComponent* em = markerObject->AddComponent<Spark::MaterialComponent>()) {
            em->SetEmissive({0.95F, 0.42F, 0.12F}, 2.1F);
            em->SetRoughness(0.38F);
        }
        roots.PushBack(markerObject);

        editCursorObject = w.CreateGameObject();
        editCursorObject->GetName() = Spark::Utf8String("TerrainEditCursor");
        editCursorTransform = editCursorObject->AddComponent<Spark::TransformComponent>();
        editCursorTransform->SetUniformScale(0.34F);
        editCursorTransform->SetTranslation({0.0F, -2000.0F, 0.0F});
        editCursorObject->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.2F, 0.95F, 1.0F});
        if (Spark::MaterialComponent* m = editCursorObject->AddComponent<Spark::MaterialComponent>()) {
            m->SetEmissive({0.18F, 0.92F, 1.0F}, 4.2F);
            m->SetRoughness(0.25F);
            editCursorMaterial = m;
        }
        roots.PushBack(editCursorObject);

        AddPointLight(w, {92.0F, 52.0F, 78.0F}, {0.4F, 0.75F, 1.0F}, 7.0F, 260.0F);
        AddPointLight(w, {-105.0F, 34.0F, -72.0F}, {1.0F, 0.55F, 0.28F}, 5.8F, 240.0F);

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("TerrainFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(12.0F, 12.0F);
        fpsText->SetFontSizePixels(20.0F);
        fpsText->SetColor({0.9F, 0.95F, 1.0F});
        fpsText->SetText(Spark::Utf8String(
                "Terrain — cyan dot = aim · LMB raise / RMB lower · R reset · F1 mouse"));
        roots.PushBack(fpsHudObject);

        context.GetInput().SetCursorCaptured(true);
        camera.position = {0.0F, 62.0F, 228.0F};
        camera.SnapLookAt({0.0F, 14.0F, 0.0F});
    }

void TerrainDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        terrainObject = nullptr;
        terrainComp = nullptr;
        editCursorObject = nullptr;
        editCursorTransform = nullptr;
        editCursorMaterial = nullptr;
        markerObject = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
        unitCubeAsset.Reset();
        groundTex.Reset();
    }

void TerrainDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        Spark::IInput& in = context.GetInput();
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsCursorCaptured()) {
            if (timing.frameIndex > 0) {
                camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
            }
            camera.ProcessMovement(in, timing.deltaTimeSeconds);
        }
        if (terrainComp != nullptr && terrainObject != nullptr) {
            if (in.IsKeyPressedThisFrame(GLFW_KEY_R)) {
                terrainComp->ResetHeightsToProcedural(*terrainObject);
            }

            Spark::Vector3 ro{};
            Spark::Vector3 rd{};
            bool haveRay = false;
            if (in.IsCursorCaptured()) {
                ro = camera.position;
                rd = camera.Forward();
                haveRay = rd.LengthSquared() > 1.0e-12F;
            } else {
                float mx = 0.0F;
                float my = 0.0F;
                in.GetCursorFramebufferPixels(mx, my, fbW, fbH);
                const Spark::Matrix4 view = camera.ViewMatrix();
                const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
                const Spark::Matrix4 proj =
                        Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 900.0F);
                const Spark::Matrix4 vp = proj * view;
                Spark::Matrix4 invVp{};
                if (vp.TryInvert(invVp)) {
                    haveRay = TerrainScreenToWorldRay(fbW, fbH, mx, my, invVp, ro, rd);
                }
            }

            Spark::Vector3 hit{};
            const bool aimHit = haveRay && terrainComp->TryRaycastWorld(*terrainObject, ro, rd, 750.0F, hit);

            if (editCursorTransform != nullptr) {
                if (aimHit) {
                    editCursorTransform->SetTranslation(Spark::Vector3{hit.x, hit.y + 0.18F, hit.z});
                    if (editCursorMaterial != nullptr) {
                        const bool leftDown = in.IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
                        const bool rightDown = in.IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
                        if (leftDown) {
                            editCursorMaterial->SetEmissive({0.35F, 1.0F, 0.48F}, 5.8F);
                        } else if (rightDown) {
                            editCursorMaterial->SetEmissive({1.0F, 0.36F, 0.2F}, 5.8F);
                        } else {
                            editCursorMaterial->SetEmissive({0.18F, 0.92F, 1.0F}, 4.2F);
                        }
                    }
                } else {
                    editCursorTransform->SetTranslation({0.0F, -2000.0F, 0.0F});
                }
            }

            const bool left = in.IsMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
            const bool right = in.IsMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT);
            if ((left || right) && aimHit) {
                const float strength = 48.0F * timing.deltaTimeSeconds;
                const float delta = left ? strength : -strength;
                terrainComp->ApplyHeightBrushWorld(*terrainObject, hit, 10.0F, delta);
            }
        }
        if (fpsText != nullptr) {
            const float dt = timing.deltaTimeSeconds;
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            fpsText->SetText(Spark::Utf8String(
                    std::format(
                            "Terrain — {:.0f} FPS — cyan dot = aim · LMB/RMB sculpt · R reset · F1 mouse",
                            static_cast<double>(fpsSmoothed))
                            .c_str()));
        }
    }

void TerrainDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

        const Spark::Matrix4 proj =
                Spark::Matrix4::PerspectiveVulkan(Spark::DegreesToRadians(60.0F), aspect, 0.12F, 900.0F);
        const Spark::Matrix4 view = camera.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = camera.position;
        params.lightDirectionWorld = Spark::Vector3{0.38F, 0.82F, 0.35F}.Normalized();
        params.lightColor = {1.0F, 0.97F, 0.9F};
        params.lightIntensity = 0.95F;
        params.ambientColor = {0.10F, 0.12F, 0.14F};
        params.lightingProfile = Spark::SceneLightingProfile::Outdoor;
        params.useTimeOfDay = true;
        params.timeOfDay = 0.48F;
        params.shadowCascadeFar = 900.0F;

        params.draws.Clear();
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
        params.draws.Reserve(24);

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
        drawList.Reserve(16);
        scene.ForEachDrawable([&](Spark::GameObject* /*obj*/, const Spark::MeshComponent& mc,
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
            item.albedo = alb;
            drawList.PushBack(item);
        });

        StableSortDrawItems(drawList);
        for (std::size_t di = 0; di < drawList.GetSize(); ++di) {
            params.draws.PushBack(drawList[di]);
        }

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

void TerrainDemo::AddPointLight(
            Spark::GameWorld& w,
            Spark::Vector3 position,
            Spark::Vector3 color,
            float intensity,
            float range)
{
        Spark::GameObject* light = w.CreateGameObject();
        light->GetName() = Spark::Utf8String("TerrainPointLight");
        Spark::TransformComponent* tr = light->AddComponent<Spark::TransformComponent>();
        tr->SetTranslation(position);
        light->AddComponent<Spark::PointLightComponent>(color, intensity, range)->SetCastsShadow(true);
        roots.PushBack(light);
    }
}  // namespace Spark
