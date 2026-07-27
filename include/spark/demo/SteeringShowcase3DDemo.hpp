#pragma once

#include "spark/ai/GameAiSubsystem.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/ai/NavMeshAgentComponent.hpp"
#include "spark/ecs/components/ai/PatrolPathComponent.hpp"
#include "spark/ai/steering/SteeringBehaviors3D.hpp"
#include "spark/ai/steering/SteeringEnvironment3D.hpp"
#include "spark/demo/ShellDemoInternalIncludes.hpp"
#include "spark/demo/ShellDemoSceneUtil.hpp"
#include "spark/demo/DemoProceduralSound.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/ecs/components/lighting/PointLightComponent.hpp"
#include "spark/ecs/components/rendering/SkyComponent.hpp"
#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/scene/Scene.hpp"

#include <cmath>
#include <format>

namespace Spark {

enum class SteeringShowcaseKind : std::uint8_t {
    Seek = 0,
    Flee,
    Arrive,
    Pursuit,
    Evade,
    Wander,
    ObstacleAvoidance,
    WallAvoidance,
    Interpose,
    Hide,
    PathFollowing,
    OffsetPursuit,
    Separation,
    Alignment,
    Cohesion,
    Flocking,
    CombineSeekObstacle,
    CombineFleeWall,
    CombineArriveWander,
    CombinePursuitObstacle,
    CombineFlockingObstacle,
    Count
};

/**
 * 3D fly camera + ground; shows only the actors relevant to the current behavior.
 * Keys: <c>[</c> / <c>]</c> cycle mode; <c>1</c>–<c>9</c> Seek…Interpose; <c>0</c> Hide; <c>-</c> / <c>=</c> Path / Offset;
 * <c>QWER</c> Separation…Flocking; <c>TYUI</c> first four combines; <c>O</c> Flocking+obstacle combine.
 * Arrow keys move the magenta target when that role is visible (<c>W</c> is reserved for Alignment); <c>F1</c> mouse look.
 */
class SteeringShowcase3DDemo {
public:
    void Load(GameWorld& w, IEngineContext& context);


    void Unload(GameWorld& w);


    void Simulate(const FrameTiming& timing, IEngineContext& context, GameWorld& world);


    void Render(Scene& scene, GameWorld& world, IEngineContext& context);


private:
    [[nodiscard]] static bool DrawableCollisionMesh(const GameObject* o) noexcept;


    /** Kinematic overlap resolution: unit cubes vs spheres (mesh radii match CreateSkySphere / CreateUnitCube). */
    void ResolveSteeringDemoCollisions() noexcept;


    /** Restores actors to the same layout as a fresh load (orbit timers zero, velocities cleared). */
    void ResetSteeringDemoEntitiesToInitial() noexcept;


    static void SetActorMesh(GameObject* go, const SharedPtr<Mesh>& mesh, const bool visible) noexcept;


    void SetFlockMeshesVisible(const bool visible) noexcept;


    void SetObstacleMeshesVisible(const bool visible) noexcept;


    /** Modes where the magenta target is visible and should respond to arrow keys. */
    [[nodiscard]] static bool UsesInteractiveTarget(const SteeringShowcaseKind m) noexcept;


    void ApplySteeringShowcaseVisibility() noexcept;


    [[nodiscard]] static const char* ModeName(const SteeringShowcaseKind m) noexcept;


    [[nodiscard]] static Vector3 ClampHorizSpeed(const Vector3& v, const float maxSp) noexcept;


    [[nodiscard]] static Vector3 ComputeSteeringForMode(
            const SteeringShowcaseKind mode,
            const Vector3& pos,
            const Vector3& vel,
            const SteeringEnvironment3D& env,
            AiBlackboard& board);


    FlyCamera camera{};
    Array<GameObject*> roots{};
    TextOverlayComponent* hudText = nullptr;
    SharedPtr<Mesh> skyMesh{};
    SharedPtr<Mesh> groundMesh{};
    SharedPtr<Mesh> sphereMesh{};
    SharedPtr<Mesh> cubeMesh{};
    GameObject* targetGo = nullptr;
    GameObject* pursuerGo = nullptr;
    GameObject* secondaryGo = nullptr;
    GameObject* leaderGo = nullptr;
    GameObject* primaryGo = nullptr;
    GameObject* ecsPatrolPathGo = nullptr;
    GameObject* ecsPatrolAgentGo = nullptr;
    Array<GameObject*> flockGos{};
    Array<Vector3> flockVels{};
    Array<Vector3> obstacleCenters{};
    Array<float> obstacleRadii{};
    Array<GameObject*> obstacleGos{};
    Array<Vector3> pathPoints{};
    int pathIndex = 0;
    Vector3 primaryVel{Vector3::Zero};
    AiBlackboard wanderBoard{};
    SteeringShowcaseKind mode = SteeringShowcaseKind::Seek;
    float orbitPursuer = 0.0F;
    float orbitSecondary = 0.0F;
    float orbitLeader = 0.0F;

};

}  // namespace Spark
