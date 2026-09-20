#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/mesh/Mesh.hpp"
#include "spark/scene/water/WaterWavePreset.hpp"

namespace Spark {

constexpr float kWaterLakeDemoFovYDeg = 60.0F;

/**
 * W1–W3 showcase: beach island + infinite Gerstner ocean at Y=0.
 * Uses the dedicated water shader pass, SSR, and fly-camera clipmap follow.
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
    void SyncSkyToCamera();

    Array<GameObject*> roots{};
    FlyCamera camera{};
    DemoHelpHud helpHud{};

    GameObject* waterObject = nullptr;
    WaterBodyComponent* waterBody = nullptr;
    GameObject* skyObject = nullptr;
    SharedPtr<Mesh> skyMesh{};

    WaterWavePreset wavePreset{WaterWavePresetId::CalmLake};
    float timeScale = 1.0F;
    float sceneTime = 0.0F;
    bool ssaoEnabled = false;
    bool skyHasHdr = false;
};

}  // namespace Spark
