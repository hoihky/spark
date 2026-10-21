#pragma once

#include "spark/physics/PhysicsQueries3D.hpp"
#include "spark/scene/query/SceneRaycast.hpp"

namespace Spark {

class GameWorld;
class Scene;

/**
 * Unified closest-hit raycast across render meshes (scene pick) and physics colliders.
 */
class ScenePhysicsRaycast3D {
public:
    explicit ScenePhysicsRaycast3D(const Scene& sceneIn) : scene(sceneIn) {}

    void SetPhysicsQueries(PhysicsQueryWorld3D* queriesIn) noexcept { physicsQueries = queriesIn; }

    [[nodiscard]] bool RaycastClosest(
            GameWorld& world,
            const SceneRay& ray,
            SceneRaycastHit& outHit,
            const SceneRaycastOptions& sceneOptions,
            const PhysicsQueryFilter3D& physicsFilter,
            GameObject* ignoreDynamic) const;

private:
    const Scene& scene;
    PhysicsQueryWorld3D* physicsQueries = nullptr;
};

}  // namespace Spark
