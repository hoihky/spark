#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/scene/water/WaterWavePreset.hpp"

namespace Spark {

constexpr float kWaterLakeDemoFovYDeg = 60.0F;

/**
 * W1 showcase: infinite Gerstner ocean at Y=0 with a contrasting procedural sky.
 * Uses the dedicated water shader pass and fly-camera clipmap follow.
 */
class WaterLakeDemo {
public:
    void Load(GameWorld& world, IEngineContext& context);
    void Unload(GameWorld& world);
    void Simulate(const FrameTiming& timing, IEngineContext& context);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    void CycleWavePreset();
    void SyncOceanClipmapCamera();

    Array<GameObject*> roots{};
    FlyCamera camera{};
    DemoHelpHud helpHud{};

    GameObject* waterObject = nullptr;
    WaterBodyComponent* waterBody = nullptr;

    WaterWavePreset wavePreset{WaterWavePresetId::StormySea};
    float timeScale = 10.0F;
    float sceneTime = 0.0F;
    bool ssaoEnabled = false;
};

}  // namespace Spark
