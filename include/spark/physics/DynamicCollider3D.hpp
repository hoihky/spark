#pragma once

#include "spark/physics/Collision3D.hpp"

namespace Spark {

class CapsuleCollider3DComponent;
class GameObject;
class Rigidbody3DComponent;
class SphereCollider3DComponent;
class TransformComponent;

/** Runtime shape tag for a dynamic rigidbody collider (sphere or capsule). */
enum class DynamicCollider3DShape : std::uint8_t {
    Sphere = 0,
    Capsule = 1,
};

/**
 * World-space snapshot of a dynamic collider used by <c>SimulatePhysics3D</c>.
 * <c>bounds</c> is the broad-phase envelope; narrow-phase fields depend on <c>shape</c>.
 */
struct DynamicCollider3DSim {
    DynamicCollider3DShape shape = DynamicCollider3DShape::Sphere;
    Vector3 sphereCenter{Vector3::Zero};
    float sphereRadius = 0.5F;
    CollisionCapsule3 capsule{};
    CollisionAabb3 bounds{};
};

/** Owning ECS handles for one integrated dynamic body. */
struct DynamicBody3D {
    GameObject* obj = nullptr;
    Rigidbody3DComponent* rb = nullptr;
    TransformComponent* tr = nullptr;
    SphereCollider3DComponent* sphere = nullptr;
    CapsuleCollider3DComponent* capsule = nullptr;
    DynamicCollider3DSim sim{};
};

void BuildDynamicCollider3DSimFromSphere(
        GameObject& owner,
        const SphereCollider3DComponent& collider,
        DynamicCollider3DSim& out) noexcept;

void BuildDynamicCollider3DSimFromCapsule(
        GameObject& owner,
        const CapsuleCollider3DComponent& collider,
        DynamicCollider3DSim& out) noexcept;

/** Point whose translation tracks the owning transform (sphere center or capsule midpoint). */
[[nodiscard]] Vector3 GetDynamicCollider3DTrackingPoint(const DynamicCollider3DSim& sim) noexcept;

void TranslateDynamicCollider3DSim(DynamicCollider3DSim& sim, const Vector3& delta) noexcept;

void RebuildDynamicCollider3DBounds(DynamicCollider3DSim& sim) noexcept;

[[nodiscard]] bool DynamicCollider3DOverlapsStatic(
        const DynamicCollider3DSim& dynamic, const StaticCollider3DSim& staticCollider) noexcept;

[[nodiscard]] bool ComputeDynamicStaticCollider3Contact(
        const DynamicCollider3DSim& dynamic,
        const StaticCollider3DSim& staticCollider,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;

[[nodiscard]] bool SeparateDynamicCollider3DFromStatic(
        DynamicCollider3DSim& dynamic, const StaticCollider3DSim& staticCollider) noexcept;

[[nodiscard]] bool ComputeDynamicDynamicCollider3Contact(
        const DynamicCollider3DSim& a,
        const DynamicCollider3DSim& b,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration) noexcept;

[[nodiscard]] bool SeparateDynamicDynamicCollider3Position(
        DynamicCollider3DSim& a,
        DynamicCollider3DSim& b,
        float inverseMassA,
        float inverseMassB) noexcept;

/** Isotropic inverse inertia scalar (sphere uses <c>2/5 m r²</c>; capsule uses a bounding-sphere approximation). */
[[nodiscard]] float EffectiveDynamicCollider3DInverseInertia(
        const Rigidbody3DComponent& rb, const DynamicCollider3DSim& sim) noexcept;

/**
 * Contact point on body <c>a</c> for torque (surface point facing <c>b</c>).
 * For spheres this is center + normal * radius; for capsules it uses the segment closest approach.
 */
void ComputeDynamicDynamicContactPointOnA(
        const DynamicCollider3DSim& a,
        const DynamicCollider3DSim& b,
        float normalX,
        float normalY,
        float normalZ,
        Vector3& outPointOnA) noexcept;

struct TriggerVolume3DSettings;

/**
 * Builds a probe snapshot for <c>SimulateTriggerVolumes3D</c>. Returns false when the object has no supported shape.
 */
[[nodiscard]] bool TryBuildTriggerProbe3DFromObject(
        GameObject& object,
        const TriggerVolume3DSettings& settings,
        DynamicCollider3DSim& outProbe) noexcept;

}  // namespace Spark
