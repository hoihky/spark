#pragma once

#include "spark/core/Array.hpp"
#include "spark/physics/3d/PhysicsWorld3D.hpp"
#include "spark/physics/3d/colliders/Collider3D.hpp"
#include "spark/physics/3d/colliders/DynamicBody3D.hpp"
#include "spark/physics/3d/SpatialHashGrid3D.hpp"

namespace Spark {

class PhysicsContactBroadcaster3D;

/** Static and dynamic contact resolution for 3D rigidbodies. */
class ContactResolver3D {
public:
    static void ApplyStaticNormalImpulses(
            Array<DynamicBody3D>& bodies,
            const Array<Collider3D>& colliders,
            SpatialHashGrid3D& broadPhase,
            Array<std::uint32_t>& scratch,
            float substepDt,
            const PhysicsWorld3DSettings& settings,
            PhysicsContactBroadcaster3D* contactBroadcaster = nullptr) noexcept;

    static void ResolvePenetrations(
            Array<DynamicBody3D>& bodies,
            const Array<Collider3D>& colliders,
            SpatialHashGrid3D& broadPhase,
            Array<std::uint32_t>& scratch,
            const PhysicsWorld3DSettings& settings) noexcept;

    static void ResolveDynamicPairVelocities(
            Array<DynamicBody3D>& bodies,
            float substepDt,
            const PhysicsWorld3DSettings& settings,
            PhysicsContactBroadcaster3D* contactBroadcaster = nullptr) noexcept;
};

}  // namespace Spark
