#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

enum class RigidbodyBodyType3D : std::uint8_t {
    Kinematic = 0,
    Static = 1,
    Dynamic = 2,
};

/**
 * 3D velocity-based body. Dynamic spheres with <c>SphereCollider3DComponent</c> are integrated by
 * <c>SimulatePhysics3D</c> (static boxes, dynamic sphere pairs, optional joints; combines with
 * <c>PhysicsMaterial3DComponent</c> on surfaces). Spherical inertia uses <c>I = 2/5 m r²</c> from collider
 * scale unless <c>inverseInertiaTensorScale</c> overrides the scalar inverse.
 */
class Rigidbody3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::Rigidbody3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit Rigidbody3DComponent(
            RigidbodyBodyType3D bodyType = RigidbodyBodyType3D::Dynamic,
            float gravityScaleIn = 1.0F) noexcept
            : bodyType(bodyType), gravityScale(gravityScaleIn) {}

    /** Linear damping coefficient (1/s); applied as exponential decay each substep. */
    [[nodiscard]] float GetLinearDamping() const noexcept { return linearDamping; }
    void SetLinearDamping(const float d) noexcept { linearDamping = d; }

    /** Angular damping (1/s); exponential decay on angular velocity each substep. */
    [[nodiscard]] float GetAngularDamping() const noexcept { return angularDamping; }
    void SetAngularDamping(const float d) noexcept { angularDamping = d; }

    /**
     * Optional override for isotropic inverse inertia (1 / I) used by the sphere solver.
     * When zero, <c>SimulatePhysics3D</c> uses solid-sphere <c>I = 2/5 m r²</c> from mass and collider radius.
     */
    [[nodiscard]] float GetInverseInertiaTensorScale() const noexcept { return inverseInertiaTensorScale; }
    void SetInverseInertiaTensorScale(const float invI) noexcept { inverseInertiaTensorScale = invI >= 0.0F ? invI : 0.0F; }

    /** Reciprocal mass (1 kg⁻¹ default = 1). Use zero only for immovable custom logic; dynamics should stay positive. */
    [[nodiscard]] float GetInverseMass() const noexcept { return inverseMass; }
    void SetInverseMass(const float invM) noexcept { inverseMass = invM > 0.0F ? invM : 0.0F; }

    [[nodiscard]] RigidbodyBodyType3D GetBodyType() const noexcept { return bodyType; }
    void SetBodyType(RigidbodyBodyType3D t) noexcept { bodyType = t; }

    [[nodiscard]] float GetGravityScale() const noexcept { return gravityScale; }
    void SetGravityScale(float g) noexcept { gravityScale = g; }

    /** Normal-direction bounce in [0,1]; 0 = inelastic (slide/stick along normal), 1 = full elastic reflection. */
    [[nodiscard]] float GetRestitution() const noexcept { return restitution; }
    void SetRestitution(float e) noexcept { restitution = e; }

    [[nodiscard]] const Vector3& GetVelocity() const noexcept { return velocity; }
    void SetVelocity(const Vector3& v) noexcept { velocity = v; }

    [[nodiscard]] const Vector3& GetAngularVelocity() const noexcept { return angularVelocity; }
    void SetAngularVelocity(const Vector3& w) noexcept { angularVelocity = w; }

private:
    RigidbodyBodyType3D bodyType = RigidbodyBodyType3D::Dynamic;
    float gravityScale = 1.0F;
    float restitution = 0.0F;
    float linearDamping = 0.0F;
    float angularDamping = 0.0F;
    float inverseMass = 1.0F;
    /** When > 0, overrides auto solid-sphere inverse inertia scalar. */
    float inverseInertiaTensorScale = 0.0F;
    Vector3 velocity{Vector3::Zero};
    Vector3 angularVelocity{Vector3::Zero};
};

}  // namespace Spark
