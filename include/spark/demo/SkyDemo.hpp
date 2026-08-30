#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/rendering/PostProcessVolumeComponent.hpp"

namespace Spark {

/** Vertical FOV (degrees) for sky demo projection — must match PerspectiveVulkan in Render(). */
constexpr float kSkyDemoFovYDeg = 60.0F;

/** Fly-camera HDR sky showcase (box / dome / plane) with lit ground and directional shadows. */
class SkyDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    void ApplySkyModeVisuals();


    void UpdateSkyTransform(int fbW, int fbH);


    Spark::Array<Spark::GameObject*> roots{};
    Spark::FlyCamera camera{};
    int skyModeIndex = 0;

    Spark::SharedPtr<Spark::Mesh> skyBoxMesh;
    Spark::SharedPtr<Spark::Mesh> skyPlaneMesh;
    Spark::SharedPtr<Spark::Mesh> groundAsset;
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset;
    Spark::SharedPtr<Spark::Texture2D> skyEquirectTex;
    Spark::SharedPtr<Spark::Texture2D> groundDiffTex;
    bool skyHasEquirect = false;
    Spark::Utf8String skySourceLabel{};

    Spark::GameObject* groundObject = nullptr;
    Spark::GameObject* skyObject = nullptr;
    Spark::TransformComponent* skyTransform = nullptr;
    Spark::MeshComponent* skyMesh = nullptr;
    Spark::SkyComponent* sky = nullptr;
    Spark::MaterialComponent* skyMat = nullptr;
    Spark::PostProcessVolumeComponent* postVolume = nullptr;
    DemoHelpHud helpHud{};
    int skyLastFbW = 0;
    int skyLastFbH = 0;
    float sceneTime = 0.0F;
    bool ssaoEnabled = true;
};


}  // namespace Spark
