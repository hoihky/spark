#include "spark/demo/WaterLakeDemo.hpp"

#include "spark/config.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/ecs/components/rendering/PostProcessVolumeComponent.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/mesh/TerrainGeneratorSettings.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/water/WaterBodyExtent.hpp"
#include "spark/scene/water/WaterBodyMode.hpp"
#include "spark/scene/water/WaterRenderingProfile.hpp"
#include "spark/scene/water/WaterScreenSpaceReflectionSettings.hpp"
#include "spark/scene/water/WaterSurfaceMeshSettings.hpp"

#include <algorithm>
#include <cstdio>

namespace Spark {
namespace {

bool TryLoadSkyHdr(SharedPtr<Texture2D>& outTex) {
    Texture2D decoded;
    if (Texture2D::TryLoadFromFile(SPARK_SKY_TEXTURE_PATH, decoded)) {
        outTex = MakeShared<Texture2D>(MoveTemp(decoded));
        return true;
    }
    Utf8String altPath(SPARK_ASSETS_DIR);
    altPath.AppendUtf8("/textures/sky/equirect_sky_1k.hdr");
    if (Texture2D::TryLoadFromFile(altPath.CStr(), decoded)) {
        outTex = MakeShared<Texture2D>(MoveTemp(decoded));
        return true;
    }
    return false;
}

}  // namespace

void WaterLakeDemo::Load(GameWorld& world, IEngineContext& context) {
    roots.Clear();
    sceneTime = 0.0F;
    timeScale = 1.0F;
    wavePreset = WaterWavePreset(WaterWavePresetId::CalmLake);
    ssaoEnabled = false;
    skyHasHdr = false;
    skyObject = nullptr;

    skyMesh = MakeShared<Mesh>(Utf8String("WaterLakeSky"));
    *skyMesh = Mesh::CreateSkySphere(1.0F, 24, 48);
    SharedPtr<Texture2D> skyHdrTex;
    skyHasHdr = TryLoadSkyHdr(skyHdrTex);
    if (skyHasHdr) {
        world.RegisterTexture(skyHdrTex, "spark/demo/water_lake_sky");
    }

    skyObject = world.CreateGameObject();
    skyObject->GetName() = Utf8String("LakeSky");
    skyObject->AddComponent<TransformComponent>();
    skyObject->AddComponent<SkyComponent>(SceneSkyMode::Dome);
    skyObject->AddComponent<MeshComponent>(skyMesh, SceneMeshSlot::Custom, Vector3::One);
    if (MaterialComponent* skyMat = skyObject->AddComponent<MaterialComponent>()) {
        if (skyHasHdr) {
            skyMat->SetBaseColorTexture(skyHdrTex);
            skyMat->SetTint(Vector3::One);
        } else {
            skyMat->SetTint({0.55F, 0.78F, 0.98F});
        }
    }
    roots.PushBack(skyObject);

    WaterSurfaceMeshSettings meshSettings{};
    meshSettings.SetSubdivisionsPerAxis(96);
    meshSettings.SetTileHalfExtent(128.0F);
    meshSettings.SetRebuildMoveThreshold(32.0F);
    meshSettings.SetWorldUnitsPerTextureRepeat(16.0F);

    waterObject = world.CreateGameObject();
    waterObject->GetName() = Utf8String("OceanWater");
    waterObject->AddComponent<TransformComponent>();
    const Vector3 waterSurfaceColor{0.04F, 0.36F, 0.52F};
    waterObject->AddComponent<WaterBodyComponent>(
            WaterBodyMode::InfiniteOcean,
            0.0F,
            WaterBodyExtent::MakeInfinitePlaceholder(),
            wavePreset.GetId(),
            meshSettings,
            waterSurfaceColor);
    waterBody = waterObject->GetComponent<WaterBodyComponent>();
    if (waterBody != nullptr) {
        waterBody->SetWaveTimeScale(timeScale);
        waterBody->SetWaveAmplitudeScale(1.28F);
        waterBody->SetWaveSpeedScale(1.85F);
        waterBody->SetDeepColor({0.01F, 0.14F, 0.30F});
        waterBody->SetAbsorption(0.44F);
        waterBody->SetFoamStrength(0.62F);
        waterBody->SetDetailNormalStrength(0.58F);
        waterBody->SetShorelineFoamStrength(0.28F);
        waterBody->SetShorelineFoamMaxDepth(0.42F);
        // Preset waves default to +X; rotate so swell travels from open ocean (+X,+Z) toward the island.
        waterBody->SetSwellTravelDirectionWorld({-0.78F, -0.63F});
        WaterScreenSpaceReflectionSettings ssr{};
        ssr.SetMaxRayDistance(56.0F);
        ssr.SetMaxSteps(28);
        ssr.SetThickness(0.12F);
        ssr.SetStrength(0.72F);
        waterBody->SetSsrSettings(ssr);
    }
    if (MaterialComponent* waterMat = waterObject->AddComponent<MaterialComponent>()) {
        waterMat->SetRoughness(0.012F);
        waterMat->SetMetallic(0.0F);
        waterMat->SetTint({1.0F, 1.0F, 1.0F});
        waterMat->SetOpacity(1.0F);
    }
    roots.PushBack(waterObject);

    TerrainGeneratorSettings islandSettings{};
    islandSettings.subdivX = 128;
    islandSettings.subdivZ = 128;
    islandSettings.halfExtentX = 120.0F;
    islandSettings.halfExtentZ = 120.0F;
    islandSettings.heightScale = 7.5F;
    islandSettings.noiseScale = 0.018F;
    islandSettings.octaves = 4;
    islandSettings.persistence = 0.48F;
    islandSettings.worldUnitsPerTextureRepeat = 28.0F;
    islandSettings.seed = 0xBEAC41u;

    GameObject* island = world.CreateGameObject();
    island->GetName() = Utf8String("BeachIsland");
    island->AddComponent<TransformComponent>();
    if (TerrainComponent* terrain = island->AddComponent<TerrainComponent>(
                islandSettings, Vector3{0.78F, 0.72F, 0.54F})) {
        terrain->ResetHeightsToProcedural(*island);
        terrain->ApplyIslandFalloff(*island, 34.0F, 72.0F, -4.8F);
    }
    if (MaterialComponent* islandMat = island->AddComponent<MaterialComponent>()) {
        islandMat->SetRoughness(0.88F);
        islandMat->SetMetallic(0.0F);
    }
    roots.PushBack(island);

    // Submerged props for depth-absorption visual check (column depth ~1 m / 3 m / 8 m).
    constexpr float kAbsorptionTestCubeScale = 2.0F;
    constexpr float kSeabedY = -5.0F;
    constexpr float kPropZ = -72.0F;

    const SharedPtr<Mesh> absorptionCubeMesh = MakeShared<Mesh>(Utf8String("WaterLakeAbsorptionCube"));
    *absorptionCubeMesh = Mesh::CreateUnitCube();
    world.RegisterMesh(absorptionCubeMesh, "spark/demo/water_lake_absorption_cube");

    const SharedPtr<Mesh> seabedMesh = MakeShared<Mesh>(Utf8String("WaterLakeSeabed"));
    *seabedMesh = Mesh::CreateGroundPlane(kSceneGroundHalfExtent * 2.0F);
    world.RegisterMesh(seabedMesh, "spark/demo/water_lake_seabed");

    GameObject* seabed = world.CreateGameObject();
    seabed->GetName() = Utf8String("Seabed");
    seabed->AddComponent<TransformComponent>()->SetTranslation({0.0F, kSeabedY, kPropZ});
    seabed->AddComponent<MeshComponent>(
            seabedMesh, SceneMeshSlot::GroundPlane, Vector3{0.52F, 0.46F, 0.38F});
    if (MaterialComponent* seabedMat = seabed->AddComponent<MaterialComponent>()) {
        seabedMat->SetRoughness(0.82F);
        seabedMat->SetMetallic(0.0F);
        seabedMat->SetShadowCastOverride(false);
    }
    roots.PushBack(seabed);

    struct AbsorptionMarker {
        float centerY;
        float centerX;
        Vector3 albedo;
    };
    const AbsorptionMarker markers[] = {
            {-1.0F, -10.0F, {0.98F, 0.42F, 0.32F}},
            {-3.0F, 0.0F, {0.98F, 0.88F, 0.28F}},
            {-8.0F, 10.0F, {0.35F, 0.90F, 0.98F}},
    };
    for (const AbsorptionMarker& marker : markers) {
        GameObject* cube = world.CreateGameObject();
        cube->GetName() = Utf8String("AbsorptionCube");
        if (TransformComponent* transform = cube->AddComponent<TransformComponent>()) {
            transform->SetTranslation({marker.centerX, marker.centerY, kPropZ});
            transform->SetUniformScale(kAbsorptionTestCubeScale);
        }
        cube->AddComponent<MeshComponent>(
                absorptionCubeMesh, SceneMeshSlot::UnitCube, marker.albedo);
        if (MaterialComponent* cubeMat = cube->AddComponent<MaterialComponent>()) {
            cubeMat->SetRoughness(0.35F);
            cubeMat->SetMetallic(0.0F);
            cubeMat->SetShadowCastOverride(false);
        }
        roots.PushBack(cube);
    }

    GameObject* postGo = world.CreateGameObject();
    postGo->GetName() = Utf8String("PostVolume");
    postGo->AddComponent<TransformComponent>()->SetTranslation({0.0F, 2.0F, 0.0F});
    if (PostProcessVolumeComponent* post = postGo->AddComponent<PostProcessVolumeComponent>()) {
        post->SetHalfExtents({300.0F, 40.0F, 300.0F});
        post->SetSsaoEnabled(ssaoEnabled);
        post->SetExposure(1.02F);
    }
    roots.PushBack(postGo);

    helpHud.Mount(world, "Water lake");
    helpHud.SetControlHints("P wave preset | O SSAO | +/- time scale | F1 mouse | WASD fly");
    helpHud.SetDetail("Calm lake · beach SSR · absorption cubes at Z=-72");

    context.GetInput().SetCursorCaptured(true);
    camera.position = {46.0F, 6.2F, 34.0F};
    camera.moveSpeed = 10.0F;
    camera.SnapLookAt({0.0F, 1.2F, -6.0F});
    SyncOceanClipmapCamera();
    SyncSkyToCamera();
    if (waterBody != nullptr && waterObject != nullptr) {
        waterBody->RegenerateSurface(*waterObject);
    }
}

void WaterLakeDemo::Unload(GameWorld& world) {
    helpHud.Unmount(world);
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    waterObject = nullptr;
    waterBody = nullptr;
    skyObject = nullptr;
    skyMesh.Reset();
    skyHasHdr = false;
}

void WaterLakeDemo::CycleWavePreset() {
    wavePreset = WaterWavePreset::Next(wavePreset.GetId());
    if (waterBody != nullptr) {
        waterBody->SetWavePresetId(wavePreset.GetId());
    }
}

void WaterLakeDemo::SyncOceanClipmapCamera() {
    if (waterBody == nullptr) {
        return;
    }
    waterBody->SetClipmapCameraWorld(camera.position);
}

void WaterLakeDemo::SyncSkyToCamera() {
    if (skyObject == nullptr) {
        return;
    }
    if (TransformComponent* transform = skyObject->GetComponent<TransformComponent>()) {
        transform->SetTranslation(camera.position);
        transform->SetUniformScale(420.0F);
    }
}

void WaterLakeDemo::Simulate(const FrameTiming& timing, IEngineContext& context) {
    sceneTime += timing.deltaTimeSeconds * timeScale;
    IInput& input = context.GetInput();

    if (input.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        input.SetCursorCaptured(!input.IsCursorCaptured());
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_P)) {
        CycleWavePreset();
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_EQUAL) || input.IsKeyPressedThisFrame(GLFW_KEY_KP_ADD)) {
        timeScale = std::min(timeScale + 0.5F, 20.0F);
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_MINUS) || input.IsKeyPressedThisFrame(GLFW_KEY_KP_SUBTRACT)) {
        timeScale = std::max(timeScale - 0.5F, 0.0F);
    }
    if (input.IsKeyPressedThisFrame(GLFW_KEY_O)) {
        ssaoEnabled = !ssaoEnabled;
    }
    if (waterBody != nullptr) {
        waterBody->SetWaveTimeScale(timeScale);
    }

    if (input.IsCursorCaptured()) {
        if (timing.frameIndex > 0) {
            camera.AddLook(input.GetMouseDeltaX(), input.GetMouseDeltaY());
        }
        camera.ProcessMovement(input, timing.deltaTimeSeconds);
    }
    SyncOceanClipmapCamera();
    SyncSkyToCamera();

    char detail[160]{};
    std::snprintf(
            detail,
            sizeof(detail),
            "%s · time x%.2f · sky %s · SSAO %s",
            wavePreset.GetLabel(),
            timeScale,
            skyHasHdr ? "HDR" : "procedural",
            ssaoEnabled ? "on" : "off");
    helpHud.SetDetail(detail);
    helpHud.Update(timing, context);
}

void WaterLakeDemo::Render(Scene& scene, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;

    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(kWaterLakeDemoFovYDeg), aspect, 0.12F, 900.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    SceneRenderParams params{};
    params.lightingProfile = SceneLightingProfile::Outdoor;
    params.useTimeOfDay = true;
    params.timeOfDay = 0.48F;
    params.directionalShadowsEnabled = true;
    // Widen cascade 0 (~150 m split) so the 240 m island fits one tile from fly + top-down views.
    params.shadowCascadeFar = 1200.0F;
    params.shadowDepthSampleFlipV = true;
    params.punctualShadowsEnabled = false;
    params.ssaoEnabled = ssaoEnabled;
    params.iblEnabled = true;
    params.iblIntensity = 0.92F;
    // HDR sky is visible + sampled via binding 13 for water; keep terrain on procedural IBL.
    params.iblUseHdrSkyEnvironment = false;
    params.waterRenderingProfile = WaterRenderingProfile::Default;
    params.worldClearColorEnabled = !skyHasHdr;
    params.worldClearColor = {0.42F, 0.76F, 0.98F};

    FillStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.38F, 0.90F, 0.20F}.Normalized(),
            {1.0F, 0.96F, 0.86F},
            1.08F,
            Vector3{0.12F, 0.14F, 0.18F},
            false,
            {},
            {},
            sceneTime,
            params,
            SceneSpriteSortMode::SortOrderOnly,
            &scene);

    helpHud.PatchSceneRenderParams(params, world);
    context.SetSceneRenderParams(params);
}

}  // namespace Spark
