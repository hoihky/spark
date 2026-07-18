#include "spark/demo/SkyDemo.hpp"

namespace Spark {

void SkyDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        skyModeIndex = 0;

        skyBoxMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SkyEnvSphere"));
        *skyBoxMesh = Spark::Mesh::CreateSkySphere(1.0F, 20, 40);
        skyPlaneMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SkyPlaneMesh"));
        *skyPlaneMesh = Spark::Mesh::CreateSkyBillboardPlane(1.0F, 1.0F);

        skyEquirectTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("SkyEquirect"));
        skyHasEquirect = false;
        Spark::Texture2D skyDecoded;
        if (Spark::Texture2D::TryLoadFromFile(SPARK_SKY_TEXTURE_PATH, skyDecoded)) {
            *skyEquirectTex = Spark::MoveTemp(skyDecoded);
            skyHasEquirect = true;
            w.RegisterTexture(skyEquirectTex, "spark/demo/sky_equirect");
        } else {
            Spark::Utf8String alt(SPARK_ASSETS_DIR);
            alt.AppendUtf8("/textures/sky/equirect_sky_1k.hdr");
            if (Spark::Texture2D::TryLoadFromFile(alt.CStr(), skyDecoded)) {
                *skyEquirectTex = Spark::MoveTemp(skyDecoded);
                skyHasEquirect = true;
                w.RegisterTexture(skyEquirectTex, "spark/demo/sky_equirect");
            }
        }

        groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SkyDemoGround"));
        *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent);
        groundObject = w.CreateGameObject();
        groundObject->GetName() = Spark::Utf8String("Ground");
        groundObject->AddComponent<Spark::TransformComponent>();
        groundObject->AddComponent<Spark::MeshComponent>(
                groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3{0.42F, 0.48F, 0.38F});
        roots.PushBack(groundObject);

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SkyDemoCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        cubeObject = w.CreateGameObject();
        cubeObject->GetName() = Spark::Utf8String("ReferenceCube");
        {
            Spark::TransformComponent* tr = cubeObject->AddComponent<Spark::TransformComponent>();
            tr->SetTranslation({0.0F, 1.0F, 0.0F});
            tr->SetUniformScale(1.2F);
        }
        cubeObject->AddComponent<Spark::MeshComponent>(
                unitCubeAsset, Spark::SceneMeshSlot::UnitCube, Spark::Vector3{0.82F, 0.55F, 0.28F});
        roots.PushBack(cubeObject);

        skyObject = w.CreateGameObject();
        skyObject->GetName() = Spark::Utf8String("SkyEnvironment");
        skyTransform = skyObject->AddComponent<Spark::TransformComponent>();
        skyMesh = skyObject->AddComponent<Spark::MeshComponent>(skyBoxMesh, Spark::Vector3::One);
        sky = skyObject->AddComponent<Spark::SkyComponent>(Spark::SceneSkyMode::Box);
        skyMat = skyObject->AddComponent<Spark::MaterialComponent>();
        roots.PushBack(skyObject);
        ApplySkyModeVisuals();

        fpsHudObject = w.CreateGameObject();
        fpsHudObject->GetName() = Spark::Utf8String("SkyFpsHud");
        fpsText = fpsHudObject->AddComponent<Spark::TextOverlayComponent>();
        fpsText->SetScreenPosition(Spark::DemoHud::kScreenMargin, Spark::DemoHud::kScreenMargin);
        DemoHud::Apply(*fpsText);
        {
            const char* src = skyHasEquirect ? "HDR equirect" : "procedural (HDR file missing)";
            fpsText->SetText(Spark::Utf8String(
                    std::format("Sky demo — {} — TAB: Box / Dome / Plane", src).c_str()));
        }
        roots.PushBack(fpsHudObject);

        context.GetInput().SetCursorCaptured(true);
        camera.position = {0.0F, 4.5F, 14.0F};
        camera.SnapLookAt({0.0F, 1.2F, 0.0F});
    }

void SkyDemo::Unload(Spark::GameWorld& w)
{
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        groundObject = nullptr;
        cubeObject = nullptr;
        skyObject = nullptr;
        skyTransform = nullptr;
        skyMesh = nullptr;
        sky = nullptr;
        skyMat = nullptr;
        fpsHudObject = nullptr;
        fpsText = nullptr;
    }

void SkyDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        Spark::IInput& in = context.GetInput();
        if (in.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
            in.SetCursorCaptured(!in.IsCursorCaptured());
        }
        if (in.IsCursorCaptured()) {
            if (timing.frameIndex > 0) {
                camera.AddLook(in.GetMouseDeltaX(), in.GetMouseDeltaY());
            }
            camera.ProcessMovement(in, timing.deltaTimeSeconds);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_TAB)) {
            skyModeIndex = (skyModeIndex + 1) % 3;
            ApplySkyModeVisuals();
        }

        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        skyLastFbW = fbW;
        skyLastFbH = fbH;
        UpdateSkyTransform(fbW, fbH);

        if (fpsText != nullptr) {
            const float dt = timing.deltaTimeSeconds;
            const float instant = (dt > 1.0e-6F) ? (1.0F / dt) : 0.0F;
            if (timing.frameIndex < 2U) {
                fpsSmoothed = instant;
            } else {
                fpsSmoothed = fpsSmoothed * 0.88F + instant * 0.12F;
            }
            const char* label = "SkyBox";
            if (skyModeIndex == 1) {
                label = "SkyDome";
            } else if (skyModeIndex == 2) {
                label = "SkyPlane";
            }
            const char* src = skyHasEquirect ? "HDR" : "procedural";
            fpsText->SetText(Spark::Utf8String(
                    std::format("{} · {} — {:.0f} FPS — TAB cycle",
                            label,
                            src,
                            static_cast<double>(fpsSmoothed))
                            .c_str()));
        }
    }

void SkyDemo::Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context)
{
        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        UpdateSkyTransform(fbW, fbH);
        const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

        const Spark::Matrix4 proj = Spark::Matrix4::PerspectiveVulkan(
                Spark::DegreesToRadians(kSkyDemoFovYDeg), aspect, 0.12F, 400.0F);
        const Spark::Matrix4 view = camera.ViewMatrix();
        const Spark::Matrix4 viewProj = proj * view;

        Spark::SceneRenderParams params{};
        params.viewProjection = viewProj;
        params.cameraPositionWorld = camera.position;
        params.lightDirectionWorld = Spark::Vector3{0.4F, 0.85F, 0.25F}.Normalized();
        params.lightColor = {1.0F, 0.97F, 0.9F};
        params.lightIntensity = 0.9F;
        params.ambientColor = {0.13F, 0.14F, 0.17F};

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

        scene.ForEachSky([&](Spark::GameObject&, const Spark::SkyComponent& sk, const Spark::MeshComponent& mc,
                                 const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            Spark::SceneDrawItem item{};
            item.mesh = Spark::SceneMeshSlot::Custom;
            item.skyMode = sk.GetSkyMode();
            item.model = world;
            item.customMesh = mc.GetMesh();
            item.albedo = sk.GetTint();
            item.textureLayer = -1;
            item.metallic = 0.0F;
            item.roughness = 1.0F;
            if (mat != nullptr && mat->GetBaseColorTexture()) {
                const Spark::Vector3& t = mat->GetTint();
                item.albedo = {item.albedo.x * t.x, item.albedo.y * t.y, item.albedo.z * t.z};
                item.textureLayer = findOrAddTexture(mat->GetBaseColorTexture());
            }
            drawList.PushBack(item);
        });

        scene.ForEachDrawable([&](Spark::GameObject* obj, const Spark::MeshComponent& mc,
                                     const Spark::MaterialComponent* mat, const Spark::Matrix4& world) {
            if (obj != nullptr && obj->GetComponent<Spark::SkyComponent>() != nullptr) {
                return;
            }
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

void SkyDemo::ApplySkyModeVisuals()
{
        if (sky == nullptr || skyMesh == nullptr || skyMat == nullptr) {
            return;
        }
        if (skyModeIndex == 0) {
            skyMesh->SetMesh(skyBoxMesh);
            sky->SetSkyMode(Spark::SceneSkyMode::Box);
            if (skyHasEquirect) {
                sky->SetTint(Spark::Vector3::One);
                skyMesh->SetAlbedo(Spark::Vector3::One);
                skyMat->SetBaseColorTexture(skyEquirectTex);
            } else {
                sky->SetTint({0.22F, 0.34F, 0.58F});
                skyMesh->SetAlbedo(sky->GetTint());
                skyMat->SetBaseColorTexture(Spark::SharedPtr<Spark::Texture2D>{});
            }
        } else if (skyModeIndex == 1) {
            // Dome shading uses per-pixel view rays (scene.frag), not dome UVs. A hemisphere mesh leaves
            // raster holes toward zenith; a full sphere matches box/plane HDR coverage.
            skyMesh->SetMesh(skyBoxMesh);
            sky->SetSkyMode(Spark::SceneSkyMode::Dome);
            if (skyHasEquirect) {
                sky->SetTint(Spark::Vector3::One);
                skyMesh->SetAlbedo(Spark::Vector3::One);
                skyMat->SetBaseColorTexture(skyEquirectTex);
            } else {
                sky->SetTint({0.28F, 0.2F, 0.42F});
                skyMesh->SetAlbedo(sky->GetTint());
                skyMat->SetBaseColorTexture(Spark::SharedPtr<Spark::Texture2D>{});
            }
        } else {
            // Textured plane: same equirect as box/dome, but a finite billboard leaves gaps when the view
            // rotates (no fragments outside the quad). Use the env sphere for HDR; keep the quad for
            // procedural (shader uses vTexCoord on the plane).
            skyMesh->SetMesh(skyHasEquirect ? skyBoxMesh : skyPlaneMesh);
            sky->SetSkyMode(Spark::SceneSkyMode::Plane);
            if (skyHasEquirect) {
                sky->SetTint(Spark::Vector3::One);
                skyMesh->SetAlbedo(Spark::Vector3::One);
                skyMat->SetBaseColorTexture(skyEquirectTex);
            } else {
                sky->SetTint(Spark::Vector3::One);
                skyMesh->SetAlbedo(Spark::Vector3::One);
                skyMat->SetBaseColorTexture(Spark::SharedPtr<Spark::Texture2D>{});
            }
        }
        UpdateSkyTransform(skyLastFbW, skyLastFbH);
    }

void SkyDemo::UpdateSkyTransform(int fbW, int fbH)
{
        if (skyTransform == nullptr) {
            return;
        }
        if (skyModeIndex < 2 || (skyModeIndex == 2 && skyHasEquirect)) {
            skyTransform->SetTranslation(camera.position);
            skyTransform->SetRotation(Spark::Quaternion::Identity);
            // Large enough that clip-space z stays inside (0,w) after sky vertex offset; sphere avoids cube seams.
            const float sc = (skyModeIndex == 1) ? 92.0F : 98.0F;
            skyTransform->SetUniformScale(sc);
        } else {
            const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
            // Billboard must overshoot the frustum slice: a tight quad leaves clear-color gaps / clip holes
            // when yaw/pitch (HDR uses per-pixel rays, but fragments only exist under the mesh).
            constexpr float kBackDist = 40.0F;
            constexpr float kFovMargin = 1.72F;
            const Spark::Vector3 f = camera.Forward().Normalized();
            const Spark::Vector3 center = camera.position - f * kBackDist;
            // Same handedness as FlyCamera / Matrix4::LookAt: right = f × worldUp, up = right × f.
            Spark::Vector3 r = Spark::Vector3::Cross(f, Spark::Vector3::UnitY);
            if (r.LengthSquared() < 1.0e-10F) {
                r = Spark::Vector3::Cross(f, Spark::Vector3::UnitX);
                if (r.LengthSquared() < 1.0e-10F) {
                    r = Spark::Vector3::UnitX;
                }
            }
            r = r.Normalized();
            const Spark::Vector3 u = Spark::Vector3::Cross(r, f).Normalized();
            const Spark::Quaternion q = QuaternionFromRotationColumns(r, u, f).Normalized();
            skyTransform->SetTranslation(center);
            skyTransform->SetRotation(q);
            const float tanHalf = std::tan(Spark::DegreesToRadians(kSkyDemoFovYDeg) * 0.5F);
            const float halfH = tanHalf * kBackDist * kFovMargin;
            const float halfW = halfH * aspect;
            skyTransform->SetScale({halfW, halfH, 1.0F});
        }
    }
}  // namespace Spark
