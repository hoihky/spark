#pragma once

#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoMode.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"

namespace Spark {

constexpr float kTimeOfDayDemoFovYDeg = 60.0F;

/** Outdoor scene with animated <c>SceneRenderParams::timeOfDay</c> (sunrise → noon → sunset → night → loop). */
class TimeOfDayDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);

    void Unload(Spark::GameWorld& w);

    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context);

    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    void UpdateSkyTintForTime(float normalizedTime);
    [[nodiscard]] bool TryPlaceCar(
            Spark::GameWorld& w,
            const char* relativePath,
            const Spark::Vector3& pos,
            float yawRadians,
            float targetMaxExtentM);

    Spark::Array<Spark::GameObject*> roots{};
    Spark::FlyCamera camera{};
    float cycleClockSeconds = 0.0F;
    float cycleDurationSeconds = 90.0F;
    float timeSpeed = 1.0F;
    bool animateTime = true;
    bool ssaoEnabled = true;
    float hudDetailClock = 0.0F;
    bool hudDetailDirty = true;

    void RefreshHudDetail(float timeNorm) noexcept;

    Spark::SharedPtr<Spark::Mesh> skyBoxMesh;
    Spark::SharedPtr<Spark::Mesh> groundAsset;
    bool carLoaded = false;

    Spark::GameObject* groundObject = nullptr;
    Spark::GameObject* skyObject = nullptr;
    Spark::TransformComponent* skyTransform = nullptr;
    Spark::SkyComponent* sky = nullptr;
    Spark::MaterialComponent* skyMat = nullptr;
    Spark::SharedPtr<Spark::Texture2D> skyEquirectTex;
    bool skyHasEquirect = false;
    DemoHelpHud helpHud{};
    Spark::TimeOfDayDriverComponent* timeDriver = nullptr;
    Spark::FogVolumeComponent* fogVolume = nullptr;
    Spark::PostProcessVolumeComponent* postVolume = nullptr;
};

}  // namespace Spark
