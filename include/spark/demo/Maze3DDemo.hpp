#pragma once

#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/ai/NavMeshAgentComponent.hpp"
#include "spark/ecs/components/ai/PatrolPathComponent.hpp"
#include "spark/ecs/components/ai/PerceptionSensorComponent.hpp"
#include "spark/ecs/components/ai/PerceptionSensorComponent.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"
#include "spark/scene/Scene.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace Spark {

/**
 * Procedural maze on XZ (Y up): static box walls (brick texture), wide corridors (~one cell ≈
 * <c>kCellWorld</c> meters per corridor cell), HDR sky, skinned human (CesiumMan / Fox),
 * <c>SimulatePhysics3D</c> torso sphere vs static boxes, gems, first-person camera.
 */
class Maze3DDemo {
public:
    static constexpr int kMazeW = 39;
    static constexpr int kMazeH = 27;
    /** One grid step in world meters; empty cell width ≈ this (walkable corridor span). */
    static constexpr float kCellWorld = 6.0F;

    void Load(Spark::GameWorld& w, Spark::IEngineContext& context);


    void Unload(Spark::GameWorld& w);


    void Simulate(const Spark::FrameTiming& timing, Spark::IEngineContext& context, Spark::GameWorld& world);


    void Render(Spark::Scene& scene, Spark::GameWorld& world, Spark::IEngineContext& context);


private:
    void ApplyMazeSkyVisuals();


    void UpdateMazeSkyTransform();


    Spark::Array<Spark::GameObject*> roots{};
    Spark::Array<Spark::GameObject*> gemObjects{};
    Spark::SharedPtr<Spark::Mesh> unitCubeAsset{};
    Spark::SharedPtr<Spark::Mesh> groundAsset{};
    Spark::SharedPtr<Spark::Texture2D> wallBrickTex{};
    Spark::SharedPtr<Spark::Mesh> skyBoxMesh{};
    Spark::SharedPtr<Spark::Texture2D> skyEquirectTex{};
    bool mazeSkyHasEquirect = false;
    Spark::GameObject* mazeSkyObject = nullptr;
    Spark::TransformComponent* mazeSkyTransform = nullptr;
    Spark::MeshComponent* mazeSkyMeshComp = nullptr;
    Spark::SkyComponent* mazeSkyComp = nullptr;
    Spark::MaterialComponent* mazeSkyMat = nullptr;

    int wallCount = 0;
    Spark::GameObject* playerGo = nullptr;
    Spark::TransformComponent* playerTr = nullptr;
    Spark::Rigidbody3DComponent* playerRb = nullptr;
    Spark::AnimatorComponent* playerAnimator = nullptr;
    Spark::Character3DAnimFsmComponent* playerCharAnimFsm = nullptr;
    bool useHumanAvatar = false;
    float humanModelYawOffset = 0.0F;
    Spark::Quaternion humanModelBindFix{Spark::Quaternion::Identity};
    Spark::Utf8String characterAvatarHudName{};

    Spark::GameObject* fpsHudObject = nullptr;
    Spark::TextOverlayComponent* fpsText = nullptr;
    Spark::GameObject* patrolPathGo = nullptr;
    Spark::GameObject* guardGo = nullptr;
    Spark::PerceptionSensorComponent* guardPerception = nullptr;
    float fpsSmoothed = 0.0F;
    int gemsCollected = 0;
    int gemsTotal = 0;
    Spark::CharacterCameraRig rig{};

};

}  // namespace Spark
