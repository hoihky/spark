#include "spark/demo/SkyDemo.hpp"

#include "spark/config.hpp"
#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/render/lighting/SceneLightingResolver.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"
#include "spark/scene/volume/RenderVolumes.hpp"

namespace Spark {
namespace {

bool TryLoadSkyHdrTexture(Spark::GameWorld& w, Spark::SharedPtr<Spark::Texture2D>& outTex, Spark::Utf8String& outLabel) {
    Spark::Texture2D decoded;
    if (Spark::Texture2D::TryLoadFromFile(SPARK_SKY_TEXTURE_PATH, decoded)) {
        outTex = Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(decoded));
        w.RegisterTexture(outTex, "spark/demo/sky_equirect");
        outLabel = Spark::Utf8String("HDR (build default)");
        return true;
    }

    static constexpr const char* kRelativeHdrPaths[] = {
            "/textures/sky/venice_sunset_1k.hdr",
            "/textures/sky/studio_small_08_1k.hdr",
            "/textures/sky/equirect_sky_1k.hdr",
    };
    static constexpr const char* kLabels[] = {
            "venice_sunset_1k",
            "studio_small_08_1k",
            "equirect_sky_1k",
    };
    static constexpr const char* kRoots[] = {SPARK_ASSETS_DIR, SPARK_BUILD_ASSETS_DIR, "assets", nullptr};

    for (std::size_t pi = 0; pi < sizeof(kRelativeHdrPaths) / sizeof(kRelativeHdrPaths[0]); ++pi) {
        for (std::size_t ri = 0; kRoots[ri] != nullptr; ++ri) {
            Spark::Utf8String path(kRoots[ri]);
            path.AppendUtf8(kRelativeHdrPaths[pi]);
            if (Spark::Texture2D::TryLoadFromFile(path.CStr(), decoded)) {
                outTex = Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(decoded));
                w.RegisterTexture(outTex, "spark/demo/sky_equirect");
                outLabel = Spark::Utf8String(kLabels[pi]);
                return true;
            }
        }
    }
    return false;
}

Spark::GameObject* SpawnReferenceCube(
        Spark::GameWorld& w,
        const Spark::SharedPtr<Spark::Mesh>& mesh,
        const Spark::Vector3& pos,
        const float uniformScale,
        const Spark::Vector3& albedo,
        const float roughness,
        const float metallic) {
    Spark::GameObject* cube = w.CreateGameObject();
    cube->GetName() = Spark::Utf8String("SkyDemoProp");
    Spark::TransformComponent* tr = cube->AddComponent<Spark::TransformComponent>();
    tr->SetTranslation(pos);
    tr->SetUniformScale(uniformScale);
    cube->AddComponent<Spark::MeshComponent>(mesh, Spark::SceneMeshSlot::UnitCube, albedo);
    if (Spark::MaterialComponent* mat = cube->AddComponent<Spark::MaterialComponent>()) {
        mat->SetRoughness(roughness);
        mat->SetMetallic(metallic);
    }
    return cube;
}

}  // namespace

void SkyDemo::Load(Spark::GameWorld& w, Spark::IEngineContext& context)
{
        roots.Clear();
        skyModeIndex = 0;
        sceneTime = 0.0F;
        ssaoEnabled = true;
        skySourceLabel = Spark::Utf8String{};

        skyBoxMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SkyEnvSphere"));
        *skyBoxMesh = Spark::Mesh::CreateSkySphere(1.0F, 32, 64);
        skyPlaneMesh = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SkyPlaneMesh"));
        *skyPlaneMesh = Spark::Mesh::CreateSkyBillboardPlane(1.0F, 1.0F);

        skyEquirectTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("SkyEquirect"));
        skyHasEquirect = TryLoadSkyHdrTexture(w, skyEquirectTex, skySourceLabel);

        groundDiffTex = Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String("SkyDemoGroundDiffuse"));
        bool groundFromTiles = false;
        if (DemoAssets::TryLoadTerrainDemoSoilTexture(*groundDiffTex)) {
            w.RegisterTexture(groundDiffTex, "spark/demo/ground_soil");
        } else {
            const DemoAssets::RandomSoilGroundResult baked =
                    DemoAssets::BuildJitteredSoilWithGrassPatchesTexture(768U, 0x5F3759DFU);
            *groundDiffTex = Spark::MoveTemp(baked.texture);
            groundFromTiles = baked.fromKenneyTiles;
            w.RegisterTexture(groundDiffTex, "spark/demo/ground_baked");
        }

        const float groundWorldSpan = DemoAssets::ProceduralTextureSpanWorldUnits(Spark::kSceneGroundHalfExtent);
        groundAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SkyDemoGround"));
        *groundAsset = Spark::Mesh::CreateGroundPlane(Spark::kSceneGroundHalfExtent, groundWorldSpan);
        w.RegisterMesh(groundAsset, "spark/demo/ground");

        groundObject = w.CreateGameObject();
        groundObject->GetName() = Spark::Utf8String("Ground");
        groundObject->AddComponent<Spark::TransformComponent>();
        groundObject->AddComponent<Spark::MeshComponent>(
                groundAsset, Spark::SceneMeshSlot::GroundPlane, Spark::Vector3::One);
        if (Spark::MaterialComponent* gm = groundObject->AddComponent<Spark::MaterialComponent>(
                    groundDiffTex, Spark::Vector3{1.0F, 0.98F, 0.94F})) {
            gm->SetRoughness(groundFromTiles ? 0.93F : 0.91F);
            gm->SetMetallic(0.0F);
        }
        roots.PushBack(groundObject);

        unitCubeAsset = Spark::MakeShared<Spark::Mesh>(Spark::Utf8String("SkyDemoCube"));
        *unitCubeAsset = Spark::Mesh::CreateUnitCube();
        w.RegisterMesh(unitCubeAsset, "spark/demo/unit_cube");

        roots.PushBack(SpawnReferenceCube(
                w, unitCubeAsset, {0.0F, 0.62F, 0.0F}, 1.25F, {0.78F, 0.52F, 0.28F}, 0.42F, 0.08F));
        roots.PushBack(SpawnReferenceCube(
                w, unitCubeAsset, {-5.5F, 0.48F, 3.2F}, 0.95F, {0.62F, 0.66F, 0.72F}, 0.55F, 0.18F));
        roots.PushBack(SpawnReferenceCube(
                w, unitCubeAsset, {4.8F, 0.56F, -3.6F}, 1.05F, {0.84F, 0.44F, 0.30F}, 0.38F, 0.12F));

        skyObject = w.CreateGameObject();
        skyObject->GetName() = Spark::Utf8String("SkyEnvironment");
        skyTransform = skyObject->AddComponent<Spark::TransformComponent>();
        skyMesh = skyObject->AddComponent<Spark::MeshComponent>(skyBoxMesh, Spark::Vector3::One);
        sky = skyObject->AddComponent<Spark::SkyComponent>(Spark::SceneSkyMode::Box);
        skyMat = skyObject->AddComponent<Spark::MaterialComponent>();
        roots.PushBack(skyObject);
        ApplySkyModeVisuals();

        Spark::GameObject* postGo = w.CreateGameObject();
        postGo->GetName() = Spark::Utf8String("SkyPostVolume");
        postGo->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 2.0F, 0.0F});
        postVolume = postGo->AddComponent<Spark::PostProcessVolumeComponent>();
        postVolume->SetHalfExtents({18.0F, 6.0F, 18.0F});
        postVolume->SetSsaoEnabled(ssaoEnabled);
        postVolume->SetExposure(1.06F);
        roots.PushBack(postGo);

        helpHud.Mount(w, "Sky");
        helpHud.SetControlHints("M cycle box / dome / plane | O SSAO | F1 mouse capture | WASD fly");
        const char* src = skyHasEquirect ? skySourceLabel.CStr() : "procedural fallback";
        helpHud.SetDetail(src);

        context.GetInput().SetCursorCaptured(true);
        camera.position = {0.0F, 4.5F, 14.0F};
        camera.SnapLookAt({0.0F, 1.2F, 0.0F});
    }

void SkyDemo::Unload(Spark::GameWorld& w)
{
        helpHud.Unmount(w);
        for (std::size_t i = 0; i < roots.GetSize(); ++i) {
            if (roots[i] != nullptr) {
                w.DestroyGameObject(roots[i]);
            }
        }
        roots.Clear();
        groundObject = nullptr;
        skyObject = nullptr;
        skyTransform = nullptr;
        skyMesh = nullptr;
        sky = nullptr;
        skyMat = nullptr;
        postVolume = nullptr;
        groundDiffTex.Reset();
        skyEquirectTex.Reset();
        skyHasEquirect = false;
        skySourceLabel = Spark::Utf8String{};
    }

void SkyDemo::Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context)
{
        sceneTime += timing.deltaTimeSeconds;
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
        if (in.IsKeyPressedThisFrame(GLFW_KEY_M)) {
            skyModeIndex = (skyModeIndex + 1) % 3;
            ApplySkyModeVisuals();
            const char* label = "SkyBox";
            if (skyModeIndex == 1) {
                label = "SkyDome";
            } else if (skyModeIndex == 2) {
                label = "SkyPlane";
            }
            const char* src = skyHasEquirect ? skySourceLabel.CStr() : "procedural fallback";
            char detail[128]{};
            std::snprintf(detail, sizeof(detail), "%s — %s — SSAO %s", label, src, ssaoEnabled ? "on" : "off");
            helpHud.SetDetail(detail);
        }
        if (in.IsKeyPressedThisFrame(GLFW_KEY_O)) {
            ssaoEnabled = !ssaoEnabled;
            if (postVolume != nullptr) {
                postVolume->SetSsaoEnabled(ssaoEnabled);
            }
            const char* label = "SkyBox";
            if (skyModeIndex == 1) {
                label = "SkyDome";
            } else if (skyModeIndex == 2) {
                label = "SkyPlane";
            }
            const char* src = skyHasEquirect ? skySourceLabel.CStr() : "procedural fallback";
            char detail[128]{};
            std::snprintf(detail, sizeof(detail), "%s — %s — SSAO %s", label, src, ssaoEnabled ? "on" : "off");
            helpHud.SetDetail(detail);
        }

        int fbW = 0;
        int fbH = 0;
        context.GetFramebufferSize(fbW, fbH);
        skyLastFbW = fbW;
        skyLastFbH = fbH;
        UpdateSkyTransform(fbW, fbH);

        helpHud.Update(timing, context);
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
        params.lightDirectionWorld = Spark::Vector3{0.42F, 0.82F, 0.28F}.Normalized();
        params.lightColor = {1.0F, 0.97F, 0.90F};
        params.lightIntensity = 1.0F;
        params.ambientColor = {0.13F, 0.14F, 0.17F};
        params.directionalShadowsEnabled = true;
        params.shadowDepthSampleFlipV = true;
        params.punctualShadowsEnabled = false;
        params.iblEnabled = false;
        params.iblUseHdrSkyEnvironment = false;
        params.ssaoEnabled = ssaoEnabled;

        Spark::ApplyRegionalRenderVolumes(world, camera.position, params);
        const Spark::ResolvedSceneLighting resolvedLighting = Spark::SceneLightingResolver::Resolve(params);
        const std::int32_t shadowFlags = Spark::DefaultShadowFlagsFor(resolvedLighting);

        params.draws.Clear();
        params.sceneTextures.Clear();
        params.sceneHdrTextures.Clear();
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

        const auto findOrAddTexture =
                [&params](const Spark::SharedPtr<Spark::Texture2D>& tex, Spark::Vector2*, Spark::Vector2*) -> std::int32_t {
            return Spark::SceneSubmitDetail::FindOrAddSceneTexture(params, tex, nullptr, nullptr);
        };

        Spark::Array<Spark::SceneDrawItem> drawList;
        drawList.Reserve(16);

        scene.ForEachSky([&](Spark::GameObject&, const Spark::SkyComponent& sk, const Spark::MeshComponent& mc,
                                 const Spark::MaterialComponent* mat, const Spark::Matrix4& worldM) {
            Spark::SceneDrawItem item{};
            Spark::SceneSubmitDetail::PopulateSkyDrawItem(item, sk, mc, mat, worldM, params);
            drawList.PushBack(item);
        });

        scene.ForEachDrawable([&](Spark::GameObject* obj, const Spark::MeshComponent& mc,
                                     const Spark::MaterialComponent* mat, const Spark::Matrix4& worldM) {
            if (obj != nullptr && obj->GetComponent<Spark::SkyComponent>() != nullptr) {
                return;
            }
            Spark::SceneDrawItem baseItem{};
            baseItem.model = worldM;
            baseItem.mesh = mc.GetSlot();
            baseItem.albedo = mc.GetAlbedo();
            baseItem.textureLayer = -1;
            baseItem.shadowFlags = shadowFlags;
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom) {
                baseItem.customMesh = mc.GetMesh();
            }
            if (mc.GetSlot() == Spark::SceneMeshSlot::GroundPlane) {
                baseItem.doubleSided = true;
            }
            const Spark::MultiMaterialComponent* multiMat =
                    obj != nullptr ? obj->GetComponent<Spark::MultiMaterialComponent>() : nullptr;
            if (mc.GetSlot() == Spark::SceneMeshSlot::Custom && mc.GetMesh() && multiMat != nullptr &&
                !mc.GetMesh()->GetSubmeshes().IsEmpty()) {
                Spark::SceneSubmitDetail::PushRigidMeshDraws(
                        drawList, baseItem, *mc.GetMesh(), mat, multiMat, params, findOrAddTexture);
                return;
            }
            Spark::SceneDrawItem item = baseItem;
            if (mat != nullptr) {
                Spark::ApplyMaterialComponentToSceneDrawItem(item, mat, &params);
                Spark::SceneSubmitDetail::ApplyAlbedoTexture(item, mat->GetBaseColorTexture(), mat->GetTint(), findOrAddTexture);
            }
            drawList.PushBack(item);
        });

        Spark::StableSortDrawItems(drawList);
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

        helpHud.PatchSceneRenderParams(params, world);
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
            const float sc = (skyModeIndex == 1) ? 96.0F : 102.0F;
            skyTransform->SetUniformScale(sc);
        } else {
            const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
            constexpr float kBackDist = 40.0F;
            constexpr float kFovMargin = 1.72F;
            const Spark::Vector3 f = camera.Forward().Normalized();
            const Spark::Vector3 center = camera.position - f * kBackDist;
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
