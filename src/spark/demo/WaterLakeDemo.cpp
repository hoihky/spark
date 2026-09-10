#include "spark/demo/WaterLakeDemo.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/rendering/PostProcessVolumeComponent.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/scene/submit/SceneSubmit.hpp"
#include "spark/scene/water/WaterBodyExtent.hpp"
#include "spark/scene/water/WaterBodyMode.hpp"
#include "spark/scene/water/WaterSurfaceMeshSettings.hpp"

#include <algorithm>
#include <cstdio>

namespace Spark {

void WaterLakeDemo::Load(GameWorld& world, IEngineContext& context) {
    roots.Clear();
    sceneTime = 0.0F;
    timeScale = 10.0F;
    wavePreset = WaterWavePreset(WaterWavePresetId::StormySea);
    ssaoEnabled = false;

    WaterSurfaceMeshSettings meshSettings{};
    meshSettings.SetSubdivisionsPerAxis(96);
    meshSettings.SetTileHalfExtent(128.0F);
    meshSettings.SetRebuildMoveThreshold(32.0F);
    meshSettings.SetWorldUnitsPerTextureRepeat(16.0F);

    waterObject = world.CreateGameObject();
    waterObject->GetName() = Utf8String("OceanWater");
    waterObject->AddComponent<TransformComponent>();
    const Vector3 waterSurfaceColor{0.55F, 0.82F, 0.96F};
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
        waterBody->SetWaveAmplitudeScale(3.5F);
        waterBody->SetWaveSpeedScale(2.0F);
    }
    if (MaterialComponent* waterMat = waterObject->AddComponent<MaterialComponent>()) {
        waterMat->SetRoughness(0.018F);
        waterMat->SetMetallic(0.0F);
        waterMat->SetTint({1.0F, 1.0F, 1.0F});
        waterMat->SetOpacity(1.0F);
    }
    roots.PushBack(waterObject);

    GameObject* postGo = world.CreateGameObject();
    postGo->GetName() = Utf8String("PostVolume");
    postGo->AddComponent<TransformComponent>()->SetTranslation({0.0F, 2.0F, 0.0F});
    if (PostProcessVolumeComponent* post = postGo->AddComponent<PostProcessVolumeComponent>()) {
        post->SetHalfExtents({300.0F, 40.0F, 300.0F});
        post->SetSsaoEnabled(ssaoEnabled);
        post->SetExposure(1.12F);
    }
    roots.PushBack(postGo);

    helpHud.Mount(world, "Water lake");
    helpHud.SetControlHints("P wave preset | O SSAO | +/- time scale | F1 mouse | WASD fly");
    helpHud.SetDetail("Open ocean · Gerstner waves · low sun for crest highlights");

    context.GetInput().SetCursorCaptured(true);
    camera.position = {0.0F, 5.5F, 26.0F};
    camera.moveSpeed = 10.0F;
    camera.SnapLookAt({14.0F, 0.0F, -42.0F});
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

    char detail[128]{};
    std::snprintf(
            detail,
            sizeof(detail),
            "preset %s · time x%.2f · SSAO %s",
            wavePreset.GetLabel(),
            timeScale,
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
    // Late afternoon: warm sky IBL contrasts with teal water; low sun catches wave crests.
    params.timeOfDay = 0.76F;
    params.directionalShadowsEnabled = false;
    params.shadowDepthSampleFlipV = true;
    params.punctualShadowsEnabled = false;
    params.ssaoEnabled = ssaoEnabled;
    params.worldClearColorEnabled = true;
    params.worldClearColor = {0.94F, 0.62F, 0.38F};

    FillStandardLitSceneFromWorld(
            world,
            context,
            viewProj,
            camera.position,
            Vector3{0.62F, 0.28F, 0.32F}.Normalized(),
            {1.0F, 0.94F, 0.82F},
            1.15F,
            Vector3{0.08F, 0.09F, 0.12F},
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
