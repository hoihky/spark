#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsMaterial3DComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SpringJoint3DComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/Scene.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Spark {

class IEngineContext;
struct SceneRenderParams;

/**
 * First-person fly camera, static ground + boxes, one dynamic sphere, and <c>PhysicsMaterial3DComponent</c> on
 * surfaces.
 */
class PhysicsBallThrow3DDemo {
public:
    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);
    void Unload(Spark::GameWorld& w);
    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);
    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);

private:
    void BuildPortableUi(Spark::IEngineContext& context, SceneRenderParams& params, const Spark::GameWorld& world);
    void ApplyTuningFromGui() noexcept;
    void ApplyCubeBounciness(const float tIn) noexcept;
    void ApplyCubeMassScale(const float massKg) noexcept;

    Spark::Array<Spark::GameObject*> roots{};
    Spark::Array<Spark::GameObject*> cubeObjects{};
    Spark::Array<bool> cubeRubber{};
    Spark::FlyCamera camera{};
    Spark::SharedPtr<Spark::Mesh> skyMesh;
    Spark::SharedPtr<Spark::Mesh> groundMesh;
    Spark::SharedPtr<Spark::Mesh> cubeMesh;
    Spark::SharedPtr<Spark::Mesh> ballMesh;
    Spark::Rigidbody3DComponent* ballRb = nullptr;
    Spark::TransformComponent* ballTr = nullptr;
    Spark::Rigidbody3DComponent* pendulumBobRb = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    float guiGravityY = -9.81F;
    float guiBallMass = 0.62F;
    float guiThrow = 12.0F;
    float guiCubeBounce = 0.48F;
    float guiCubeMass = 95.0F;
    float fpsSmoothed = 0.0F;
    PhysicsSubsystem physics{};
};

}  // namespace Spark
