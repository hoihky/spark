#pragma once

#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;

/** Vertical gravity (+Y up); typical -24. */
struct PhysicsWorld3DSettings {
    float gravityY = -24.0F;
    float maxFallSpeed = 80.0F;
    int resolveIterations = 8;
    /** Fixed substeps per frame for stability (≥1). Each substep uses <c>deltaTime / substeps</c>. */
    int substeps = 1;
    /** Soft position iterations for <c>DistanceJoint3DComponent</c> constraints per substep. */
    int jointIterations = 0;
    /**
     * Baumgarte-style normal separation velocity added after contact impulses on the first position iteration
     * (reduces residual penetration from repeated projections). Set to 0 to disable.
     */
    float baumgarteContactBias = 0.14F;
    float baumgarteMaxSeparationVelocity = 8.0F;
    float contactPenetrationSlop = 0.0015F;
    /**
     * Binary-search steps along the proposed translation vs statics (0 = disabled).
     * Uses a conservative inflated-AABB query along the motion segment to reduce tunneling.
     */
    int sweptStaticCcdBinaryIterations = 10;
    /**
     * Kinetic Coulomb friction applies only for contacts with meaningful normal approach (m/s). Static contacts use
     * pre-restitution <c>v·n</c> (negative into the surface); sphere–sphere uses positive closing speed along <c>n</c>.
     * Stops repeated position iterations from treating resting/sliding overlap as impacts (avoids tangential "glue").
     */
    float frictionImpactNormalSpeed = 0.48F;
};

/**
 * Integrates dynamic rigidbodies with <c>SphereCollider3DComponent</c> or <c>CapsuleCollider3DComponent</c> against static
 * <c>BoxCollider3DComponent</c> and <c>CapsuleCollider3DComponent</c>,
 * dynamic sphere/capsule pairs (sphere–sphere, sphere–capsule, capsule–capsule), <c>DistanceJoint3DComponent</c> constraints, and optional
 * <c>PhysicsMaterial3DComponent</c> on surfaces. Rebuilds a 3D spatial hash of statics each step.
 * Solid spheres use <c>I = 2/5 m r²</c> (or <c>Rigidbody3DComponent::inverseInertiaTensorScale</c> when set);
 * contact impulses apply torque <c>Δω = I⁻¹ (r × J)</c>. Translation is swept against statics before integration
 * when <c>sweptStaticCcdBinaryIterations > 0</c>. Restitution, friction, contact torque, and Baumgarte run only on the
 * first position iteration; later iterations apply positional separation only so contacts do not "glue" or damp bounces.
 */
void SimulatePhysics3D(GameWorld& world, const FrameTiming& timing, const PhysicsWorld3DSettings& settings = {});

}  // namespace Spark
