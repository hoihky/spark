#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

enum class RigidbodyBodyType3D : std::uint8_t {
    Kinematic = 0,
    Static = 1,
    Dynamic = 2,
};

/**
 * 3D velocity-based body. Dynamic colliders (<c>SphereCollider3DComponent</c> or <c>CapsuleCollider3DComponent</c>)
 * are integrated by <c>PhysicsWorld3D</c> / <c>PhysicsSubsystem</c> (static boxes/capsules, dynamic pairs, optional joints; combines with
 * <c>PhysicsMaterial3DComponent</c> on surfaces). Spherical inertia uses <c>I = 2/5 m r²</c> from collider
 * scale; capsules use a bounding-sphere approximation unless <c>inverseInertiaTensorScale</c> overrides the scalar inverse.
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
    SPARK_SCRIPT_BIND(get_linear_damping)
    [[nodiscard]] float GetLinearDamping() const noexcept { return linearDamping; }
    SPARK_SCRIPT_BIND(set_linear_damping)
    void SetLinearDamping(const float d) noexcept { linearDamping = d; }

    /** Angular damping (1/s); exponential decay on angular velocity each substep. */
    SPARK_SCRIPT_BIND(get_angular_damping)
    [[nodiscard]] float GetAngularDamping() const noexcept { return angularDamping; }
    SPARK_SCRIPT_BIND(set_angular_damping)
    void SetAngularDamping(const float d) noexcept { angularDamping = d; }

    /**
     * Optional override for isotropic inverse inertia (1 / I) used by the sphere solver.
     * When zero, <c>PhysicsWorld3D</c> uses solid-sphere <c>I = 2/5 m r²</c> from mass and collider radius.
     */
    SPARK_SCRIPT_BIND(get_inverse_inertia_tensor_scale)
    [[nodiscard]] float GetInverseInertiaTensorScale() const noexcept { return inverseInertiaTensorScale; }
    SPARK_SCRIPT_BIND(set_inverse_inertia_tensor_scale)
    void SetInverseInertiaTensorScale(const float invI) noexcept { inverseInertiaTensorScale = invI >= 0.0F ? invI : 0.0F; }

    /** Reciprocal mass (1 kg⁻¹ default = 1). Use zero only for immovable custom logic; dynamics should stay positive. */
    SPARK_SCRIPT_BIND(get_inverse_mass)
    [[nodiscard]] float GetInverseMass() const noexcept { return inverseMass; }
    SPARK_SCRIPT_BIND(set_inverse_mass)
    void SetInverseMass(const float invM) noexcept { inverseMass = invM > 0.0F ? invM : 0.0F; }

    SPARK_SCRIPT_BIND(get_body_type)
    [[nodiscard]] RigidbodyBodyType3D GetBodyType() const noexcept { return bodyType; }
    SPARK_SCRIPT_BIND(set_body_type)
    void SetBodyType(RigidbodyBodyType3D t) noexcept { bodyType = t; }

    SPARK_SCRIPT_BIND(get_gravity_scale)
    [[nodiscard]] float GetGravityScale() const noexcept { return gravityScale; }
    SPARK_SCRIPT_BIND(set_gravity_scale)
    void SetGravityScale(float g) noexcept { gravityScale = g; }

    /** Normal-direction bounce in [0,1]; 0 = inelastic (slide/stick along normal), 1 = full elastic reflection. */
    SPARK_SCRIPT_BIND(get_restitution)
    [[nodiscard]] float GetRestitution() const noexcept { return restitution; }
    SPARK_SCRIPT_BIND(set_restitution)
    void SetRestitution(float e) noexcept { restitution = e; }

    SPARK_SCRIPT_BIND(get_velocity)
    [[nodiscard]] const Vector3& GetVelocity() const noexcept { return velocity; }
    SPARK_SCRIPT_BIND(set_velocity)
    void SetVelocity(const Vector3& v) noexcept { velocity = v; }

    SPARK_SCRIPT_BIND(get_angular_velocity)
    [[nodiscard]] const Vector3& GetAngularVelocity() const noexcept { return angularVelocity; }
    SPARK_SCRIPT_BIND(set_angular_velocity)
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
