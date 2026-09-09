#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

class PhysicsQueryWorld3D;

/**
 * Ground sampling for foot IK (physics raycast with analytic plane fallback).
 */
class IkGroundProbe {
public:
    class Hit {
    public:
        bool hasHit = false;
        Vector3 point{Vector3::Zero};
        Vector3 normal{Vector3::UnitY};
        float distance = 0.0F;
    };

    void SetPhysicsQueryWorld(PhysicsQueryWorld3D* queries) noexcept { physicsQueries = queries; }
    void SetFallbackPlaneHeight(float height) noexcept { fallbackPlaneHeight = height; }

    [[nodiscard]] bool ProbeDown(const Vector3& originWorld, float maxDistance, Hit& outHit) const;

private:
    PhysicsQueryWorld3D* physicsQueries = nullptr;
    float fallbackPlaneHeight = 0.0F;
};

}  // namespace Spark
