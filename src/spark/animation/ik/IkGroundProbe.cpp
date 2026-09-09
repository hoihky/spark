#include "spark/animation/ik/IkGroundProbe.hpp"

#include "spark/physics/PhysicsQueries3D.hpp"

#include <algorithm>

namespace Spark {

bool IkGroundProbe::ProbeDown(const Vector3& originWorld, const float maxDistance, Hit& outHit) const {
    outHit = Hit{};
    if (maxDistance <= 0.0F) {
        return false;
    }

    const Vector3 down{0.0F, -1.0F, 0.0F};
    if (physicsQueries != nullptr) {
        PhysicsQueryFilter3D filter{};
        PhysicsRaycastHit3D physicsHit{};
        if (physicsQueries->RaycastStatics(originWorld, down, maxDistance, filter, physicsHit)) {
            outHit.hasHit = true;
            outHit.point = physicsHit.hitPoint;
            outHit.normal = physicsHit.hitNormal.Normalized();
            outHit.distance = physicsHit.distanceAlongRay;
            return true;
        }
    }

    const float rayEndY = originWorld.y - maxDistance;
    if (originWorld.y < fallbackPlaneHeight && rayEndY > fallbackPlaneHeight) {
        outHit.hasHit = true;
        outHit.distance = originWorld.y - fallbackPlaneHeight;
        outHit.point = {originWorld.x, fallbackPlaneHeight, originWorld.z};
        outHit.normal = Vector3::UnitY;
        return true;
    }
    if (originWorld.y >= fallbackPlaneHeight && rayEndY <= fallbackPlaneHeight) {
        outHit.hasHit = true;
        outHit.distance = originWorld.y - fallbackPlaneHeight;
        outHit.point = {originWorld.x, fallbackPlaneHeight, originWorld.z};
        outHit.normal = Vector3::UnitY;
        return true;
    }
    return false;
}

}  // namespace Spark
