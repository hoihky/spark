#pragma once

#include "spark/demo/DemoHelpHud.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/scene/core/Scene.hpp"

namespace Spark {

class Scene;

/**
 * Tour of Spark 3D physics: queries, triggers (stay + layers), contact hooks, carry/handoff,
 * dynamic CCD, fixed step, and replay buffer.
 */
class PhysicsShowcase3DDemo {
public:
    void Load(GameWorld& world, IEngineContext& context);
    void Unload(GameWorld& world);
    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world, Scene& scene);
    void Render(Scene& scene, GameWorld& world, IEngineContext& context);

private:
    void SpawnFastProjectile(GameWorld& world);
    void TryToggleCarry(GameWorld& world);
    void TryRaycastPick(IEngineContext& context, GameWorld& world, Scene& scene, int frameIndex);
    void UpdateCarrierAnchor() noexcept;
    void UpdateHelpDetail();

    Array<GameObject*> roots{};
    Array<GameObject*> projectiles{};
    FlyCamera camera{};
    PhysicsSubsystem physics{};
    DemoHelpHud helpHud{};

    SharedPtr<Mesh> skyMesh;
    SharedPtr<Mesh> groundMesh;
    SharedPtr<Mesh> cubeMesh;
    SharedPtr<Mesh> ballMesh;

    GameObject* player = nullptr;
    CharacterController3DComponent* playerController = nullptr;
    TransformComponent* playerTransform = nullptr;
    GameObject* crate = nullptr;
    GameObject* carrierAnchor = nullptr;
    TriggerVolume3DComponent* goalTrigger = nullptr;

    int goalStayTicks = 0;
    int lastPickFrame = -1;
    Utf8String lastPickName{};

    bool useFixedStep = true;
    bool recordingReplay = false;

    Utf8String helpDetailLine{};
};

}  // namespace Spark
