#include "spark/demo/GrassFieldTerrainDemo.hpp"

#include "spark/demo/DemoAssetLoad.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/foliage/GrassFieldComponent.hpp"
#include "spark/ecs/components/foliage/WindEnvironmentComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/TerrainComponent.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/scene/foliage/GrassBladeMesh.hpp"
#include "spark/scene/foliage/GrassFoliageAlbedoTexture.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"

#include <GLFW/glfw3.h>

namespace Spark {

namespace {

constexpr float kTerrainHalfExtent = 96.0F;
constexpr float kGrassPatchCenterX = 8.0F;
constexpr float kGrassPatchCenterZ = 12.0F;

/** Local meadow patch (not full terrain). */
constexpr float kGrassPatchHalfExtentX = 22.0F;
constexpr float kGrassPatchHalfExtentZ = 20.0F;
constexpr float kGrassPlacementRadiusMeters = 18.0F;

/** Locked F2 playtest baseline (coverage vs FPS). */
constexpr float kGrassMaxViewMeters = 24.0F;
constexpr std::uint32_t kGrassFrameInstanceBudget = SceneRenderParams::DefaultMaxGrassInstancesPerFrame;
constexpr float kGrassDensityPerSqM = 9.0F;
constexpr std::uint32_t kGrassMaxCachedPerChunk = 880U;
constexpr float kGrassChunkSizeMeters = 16.0F;

void SetupDemoCameraNearGrass(FlyCamera& flyCamera, const TerrainComponent& terrain, const GameObject& terrainObject) {
    flyCamera.moveSpeed = 10.0F;
    float groundY = 0.0F;
    if (!terrain.TrySampleHeightWorld(terrainObject, kGrassPatchCenterX, kGrassPatchCenterZ, groundY)) {
        groundY = 4.0F;
    }
    flyCamera.position = {kGrassPatchCenterX, groundY + 1.65F, kGrassPatchCenterZ};
    flyCamera.SnapLookAt({kGrassPatchCenterX + 2.0F, groundY + 0.55F, kGrassPatchCenterZ + 14.0F});
}

}  // namespace

void GrassFieldTerrainDemo::Load(GameWorld& world, IEngineContext& context) {
    (void)context;
    roots.Clear();
    sceneTime = 0.0F;
    world.GetWindSubsystem().Reset();

    SharedPtr<Texture2D> groundTex = MakeShared<Texture2D>(Utf8String("GrassFieldTerrainSoil"));
    if (!DemoAssets::TryLoadTerrainDemoSoilTexture(*groundTex)) {
        *groundTex = Texture2D::CreateVariedSoilGroundPattern(1024, 1024);
    }
    world.RegisterTexture(groundTex, "spark/demo/grass_field_terrain_soil");

    TerrainGeneratorSettings terrainSettings{};
    terrainSettings.subdivX = 96;
    terrainSettings.subdivZ = 96;
    terrainSettings.halfExtentX = kTerrainHalfExtent;
    terrainSettings.halfExtentZ = kTerrainHalfExtent;
    terrainSettings.worldUnitsPerTextureRepeat = DemoAssets::ProceduralTextureSpanWorldUnits(kTerrainHalfExtent);
    terrainSettings.heightScale = 22.0F;
    terrainSettings.noiseScale = 0.018F;
    terrainSettings.octaves = 5;
    terrainSettings.persistence = 0.52F;
    terrainSettings.lacunarity = 2.1F;
    terrainSettings.seed = 0xA7F20C31u;

    GameObject* terrainObject = world.CreateGameObject();
    terrainObject->GetName() = Utf8String("GrassFieldTerrain");
    terrainObject->AddComponent<TransformComponent>();
    TerrainComponent* terrainComponent = terrainObject->AddComponent<TerrainComponent>(terrainSettings);
    if (MaterialComponent* terrainMat = terrainObject->AddComponent<MaterialComponent>(groundTex, Vector3::One)) {
        terrainMat->SetMetallic(0.02F);
        terrainMat->SetRoughness(0.93F);
    }
    roots.PushBack(terrainObject);
    SetupDemoCameraNearGrass(camera, *terrainComponent, *terrainObject);

    windEnvironmentObject = world.CreateGameObject();
    windEnvironmentObject->GetName() = Utf8String("WindEnvironment");
    windEnvironmentObject->AddComponent<TransformComponent>();
    WindEnvironmentComponent* windEnv = windEnvironmentObject->AddComponent<WindEnvironmentComponent>();
    WindSettings wind{};
    wind.SetEnabled(true);
    wind.SetDirectionWorld({0.78F, 0.0F, 0.42F});
    wind.SetBaseSpeedMetersPerSecond(1.45F);
    wind.SetGustAmplitude(0.52F);
    wind.SetGustFrequencyHertz(0.2F);
    wind.SetTurbulence(0.24F);
    windEnv->GetSettings() = wind;
    roots.PushBack(windEnvironmentObject);

    viewTargetObject = world.CreateGameObject();
    viewTargetObject->GetName() = Utf8String("GrassViewTarget");
    viewTargetObject->AddComponent<TransformComponent>();
    roots.PushBack(viewTargetObject);

    SharedPtr<Mesh> bladeMesh = GrassBladeMesh::CreateSharedBladeMesh();
    world.RegisterMesh(bladeMesh, "spark/demo/grass_field_blade");

    SharedPtr<Texture2D> grassTex = GrassFoliageAlbedoTexture::CreateSharedBladeAlbedo();
    world.RegisterTexture(grassTex, "spark/demo/grass_field_albedo");

    GameObject* grassRoot = world.CreateGameObject();
    grassRoot->GetName() = Utf8String("TerrainGrassField");
    if (TransformComponent* grassTr = grassRoot->AddComponent<TransformComponent>()) {
        grassTr->SetTranslation({kGrassPatchCenterX, 0.0F, kGrassPatchCenterZ});
    }
    grassFieldComponent = grassRoot->AddComponent<GrassFieldComponent>();
    GrassFieldComponent* grassField = grassFieldComponent;
    grassField->SetTerrainObject(terrainObject);
    grassField->SetViewTargetObject(viewTargetObject);
    grassField->SetBladeMesh(bladeMesh);
    grassField->SetAlbedoTexture(grassTex);
    grassField->SetWindBendScale(0.44F);
    grassField->SetAlphaCutoff(0.36F);
    grassField->SetNeighborhoodRingRadius(1);
    grassField->GetBounds().SetHalfExtentsMeters(kGrassPatchHalfExtentX, kGrassPatchHalfExtentZ);
    GrassChunkScatterSettings& scatter = grassField->GetScatterSettings();
    scatter.SetChunkSizeMeters(kGrassChunkSizeMeters);
    scatter.SetPlacementRadiusMeters(kGrassPlacementRadiusMeters);
    scatter.SetDensityPerSquareMeter(kGrassDensityPerSqM);
    scatter.SetSamplesPerCell(1);
    scatter.SetMaxViewDistanceMeters(kGrassMaxViewMeters);
    scatter.SetMaxVisibleInstances(kGrassFrameInstanceBudget);
    scatter.SetMaxCachedInstancesPerChunk(kGrassMaxCachedPerChunk);
    scatter.SetDistanceFadeOuterFraction(0.28F);
    scatter.SetMaxSlopeDegrees(58.0F);
    scatter.SetAlbedoTint({0.48F, 0.94F, 0.34F});
    roots.PushBack(grassRoot);

    if (TransformComponent* viewTr = viewTargetObject->GetComponent<TransformComponent>()) {
        viewTr->SetTranslation(camera.position);
    }
    grassField->SetStreamingViewWorld(camera.position);
    grassField->PrepareForRender(*grassRoot, camera.position, kGrassFrameInstanceBudget);

    helpHud.Mount(world, "Grass field terrain");
    helpHud.SetControlHints("F1 mouse | WASD fly | +/- wind | local grass meadow");
    helpHud.SetDetail("Grass only in ~38 m patch near spawn · fly out to bare hills");
    context.GetInput().SetCursorCaptured(true);
}

void GrassFieldTerrainDemo::Unload(GameWorld& world) {
    for (std::size_t i = 0; i < roots.GetSize(); ++i) {
        if (roots[i] != nullptr) {
            world.DestroyGameObject(roots[i]);
        }
    }
    roots.Clear();
    windEnvironmentObject = nullptr;
    viewTargetObject = nullptr;
    grassFieldComponent = nullptr;
    world.GetWindSubsystem().Reset();
    helpHud.Unmount(world);
}

void GrassFieldTerrainDemo::Simulate(const FrameTiming& timing, IEngineContext& context) {
    sceneTime += timing.deltaTimeSeconds;
    IInput& input = context.GetInput();
    if (input.IsKeyPressedThisFrame(GLFW_KEY_F1)) {
        input.SetCursorCaptured(!input.IsCursorCaptured());
    }
    camera.ProcessMovement(input, timing.deltaTimeSeconds);
    if (input.IsCursorCaptured()) {
        camera.AddLook(input.GetMouseDeltaX(), input.GetMouseDeltaY());
    }

    if (viewTargetObject != nullptr) {
        if (TransformComponent* viewTr = viewTargetObject->GetComponent<TransformComponent>()) {
            viewTr->SetTranslation(camera.position);
        }
    }
    if (grassFieldComponent != nullptr) {
        grassFieldComponent->SetStreamingViewWorld(camera.position);
    }

    if (windEnvironmentObject != nullptr) {
        WindEnvironmentComponent* wind = windEnvironmentObject->GetComponent<WindEnvironmentComponent>();
        if (wind != nullptr) {
            WindSettings& settings = wind->GetSettings();
            if (input.IsKeyPressedThisFrame(GLFW_KEY_W)) {
                settings.SetEnabled(!settings.IsEnabled());
            }
            if (input.IsKeyDown(GLFW_KEY_EQUAL) || input.IsKeyDown(GLFW_KEY_KP_ADD)) {
                settings.SetBaseSpeedMetersPerSecond(
                        settings.GetBaseSpeedMetersPerSecond() + 0.9F * timing.deltaTimeSeconds);
            }
            if (input.IsKeyDown(GLFW_KEY_MINUS) || input.IsKeyDown(GLFW_KEY_KP_SUBTRACT)) {
                const float next = settings.GetBaseSpeedMetersPerSecond() - 0.9F * timing.deltaTimeSeconds;
                settings.SetBaseSpeedMetersPerSecond(next > 0.0F ? next : 0.0F);
            }
        }
    }
    helpHud.Update(timing, context);
}

void GrassFieldTerrainDemo::Render(Scene& /*scene*/, GameWorld& world, IEngineContext& context) {
    int fbW = 0;
    int fbH = 0;
    context.GetFramebufferSize(fbW, fbH);
    const float aspect = (fbH > 0) ? static_cast<float>(fbW) / static_cast<float>(fbH) : 1.0F;
    const Matrix4 proj = Matrix4::PerspectiveVulkan(DegreesToRadians(60.0F), aspect, 0.1F, 420.0F);
    const Matrix4 view = camera.ViewMatrix();
    const Matrix4 viewProj = proj * view;

    LitSceneSubmitOptions options{};
    options.lightDirectionWorld = Vector3{0.4F, 0.88F, 0.24F}.Normalized();
    options.lightColor = {1.0F, 0.98F, 0.92F};
    options.lightIntensity = 1.05F;
    options.ambientColor = {0.07F, 0.09F, 0.11F};
    options.enableParticles = false;
    options.sceneTimeSeconds = sceneTime;

    SceneRenderParams params{};
    params.maxGrassInstancesPerFrame = kGrassFrameInstanceBudget;

    if (grassFieldComponent != nullptr) {
        GameObject* grassOwner = grassFieldComponent->GetOwner();
        if (grassOwner != nullptr) {
            grassFieldComponent->PrepareForRender(*grassOwner, camera.position, params.maxGrassInstancesPerFrame);
        }
    }

    FillStandardLitSceneFromWorld(world, context, viewProj, camera.position, options, params);
    params.lightingProfile = SceneLightingProfile::Outdoor;
    params.worldClearColorEnabled = true;
    params.worldClearColor = {0.52F, 0.7F, 0.94F};
    params.directionalShadowsEnabled = false;
    params.ssaoEnabled = false;

    helpHud.PatchSceneRenderParams(params, world);
    params.Sanitize();
    context.SetSceneRenderParams(params);
}

}  // namespace Spark
