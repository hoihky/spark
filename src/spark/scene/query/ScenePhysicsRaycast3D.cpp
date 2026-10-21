#include "spark/scene/query/ScenePhysicsRaycast3D.hpp"

#include "spark/scene/core/Scene.hpp"

namespace Spark {

bool ScenePhysicsRaycast3D::RaycastClosest(
        GameWorld& world,
        const SceneRay& ray,
        SceneRaycastHit& outHit,
        const SceneRaycastOptions& sceneOptions,
        const PhysicsQueryFilter3D& physicsFilter,
        GameObject* ignoreDynamic) const {
    outHit = SceneRaycastHit{};
    if (ray.direction.LengthSquared() < 1.0e-12F) {
        return false;
    }

    const Vector3 dir = ray.direction.Normalized();
    const float segmentLength = ray.tMax - ray.tMin;
    if (segmentLength <= 0.0F) {
        return false;
    }

    const Vector3 segmentOrigin{
            ray.origin.x + dir.x * ray.tMin,
            ray.origin.y + dir.y * ray.tMin,
            ray.origin.z + dir.z * ray.tMin};

    SceneRaycastHit meshHit{};
    const bool hasMesh = scene.RaycastPick(ray, meshHit, sceneOptions);

    PhysicsRaycastHit3D physicsHit{};
    bool hasPhysics = false;
    if (physicsQueries != nullptr) {
        hasPhysics = physicsQueries->Raycast(
                world,
                segmentOrigin,
                dir,
                segmentLength,
                physicsFilter,
                ignoreDynamic,
                physicsHit);
    }

    if (!hasMesh && !hasPhysics) {
        return false;
    }

    const float meshDistance = hasMesh ? meshHit.distance : 1.0e30F;
    const float physicsDistance = hasPhysics ? (ray.tMin + physicsHit.distanceAlongRay) : 1.0e30F;

    if (hasPhysics && physicsDistance <= meshDistance) {
        outHit.object = physicsHit.owner;
        outHit.distance = physicsDistance;
        outHit.pointWorld = physicsHit.hitPoint;
        outHit.normalWorld = physicsHit.hitNormal;
        return true;
    }

    outHit = meshHit;
    return hasMesh;
}

}  // namespace Spark
