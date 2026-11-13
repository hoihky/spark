#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Kinematic attach / release between character carriers and dynamic rigidbodies (carry, throw, mount).
 */
class PhysicsBodyHandoff3D {
public:
    /** Parents <c>carried</c> to <c>carrier</c> transform; rigidbody becomes kinematic until release. */
    static void AttachRigidbodyToCarrier(
            GameWorld& world,
            GameObject& carried,
            GameObject& carrier,
            const Vector3& localOffset) noexcept;

    /** Restores dynamic simulation and applies an optional world-space impulse. */
    static void ReleaseRigidbody(GameObject& carried, const Vector3& worldImpulse) noexcept;

    /** Moves attached bodies to follow carriers (call before <c>PhysicsWorld3D::Simulate</c>). */
    static void SynchronizeAttachedBodies(GameWorld& world) noexcept;
};

}  // namespace Spark
