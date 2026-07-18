#pragma once

#include "spark/demo/DemoFoundation.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/ai/PerceptionSensorComponent.hpp"
#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/world/TimeOfDayDriverComponent.hpp"
#include "spark/scene/Scene.hpp"

#include <cstdint>

namespace Spark {

/**
 * Museum-style fly-through for P2 and extended ECS components: world volumes, physics joints,
 * AI patrol/perception, ambient audio, and health/damage. Use TAB to jump between labeled stations.
 */
class ComponentShowcaseDemo {
public:
    void Load(GameWorld& w, IEngineContext& context);

    void Unload(GameWorld& w);

    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);

    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    enum class Station : std::uint8_t {
        World = 0,
        Physics,
        Ai,
        Audio,
        Gameplay,
        Count,
    };

    void BuildSharedScene(GameWorld& w);
    void BuildWorldStation(GameWorld& w);
    void BuildPhysicsStation(GameWorld& w);
    void BuildAiStation(GameWorld& w);
    void BuildAudioStation(GameWorld& w);
    void BuildGameplayStation(GameWorld& w);

    void SnapCameraToStation(Station station) noexcept;
    void AdvanceStation(int delta) noexcept;
    [[nodiscard]] static const char* StationLabel(Station s) noexcept;
    [[nodiscard]] static const char* StationComponents(Station s) noexcept;

    DemoRootCollection roots{};
    FlyCamera camera{};
    DemoSmoothedFps fpsHud{};
    float sceneTime = 0.0F;
    float dayCycleSeconds = 0.0F;
    Station activeStation = Station::World;

    SharedPtr<Mesh> unitCube{};
    SharedPtr<Mesh> unitSphere{};
    SharedPtr<Mesh> groundMesh{};
    SharedPtr<Mesh> carMesh{};
    SharedPtr<Mesh> billboardMesh{};
    SharedPtr<Texture2D> decalTex{};

    GameObject* policyRoot = nullptr;
    TimeOfDayDriverComponent* timeDriver = nullptr;
    GameObject* probeGo = nullptr;
    TransformComponent* probeTr = nullptr;
    GameObject* patrolPathGo = nullptr;
    GameObject* guardGo = nullptr;
    PerceptionSensorComponent* guardPerception = nullptr;
    GameObject* dummyGo = nullptr;
    HealthComponent* dummyHealth = nullptr;
    DamageableComponent* dummyDamageable = nullptr;
    GameObject* listenerGo = nullptr;
    TransformComponent* listenerTr = nullptr;
    TextOverlayComponent* hudText = nullptr;
    SoundEngine* audioEngine = nullptr;
};

}  // namespace Spark
