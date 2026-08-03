#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/shapes/IShape3D.hpp"
#include "spark/physics/shapes/ShapeType3D.hpp"

namespace Spark {

class CapsuleCollider3DComponent;
class Collider3D;
class GameObject;
class Rigidbody3DComponent;
class SphereCollider3DComponent;
class TransformComponent;
struct TriggerVolume3DSettings;

/** Runtime shape tag for a dynamic rigidbody collider (sphere or capsule). */
enum class DynamicCollider3DShape : std::uint8_t {
    Sphere = 0,
    Capsule = 1,
};

/**
 * Legacy world-space snapshot of a dynamic 3D collider (kept for migration and trigger probes).
 * Prefer <c>DynamicCollider3D</c> for new simulation code.
 */
struct DynamicCollider3DSim {
    DynamicCollider3DShape shape = DynamicCollider3DShape::Sphere;
    Vector3 sphereCenter{Vector3::Zero};
    float sphereRadius = 0.5F;
    CollisionCapsule3 capsule{};
    CollisionAabb3 bounds{};
};

/**
 * Object-oriented dynamic 3D collider snapshot (Phase 4).
 * Owns an <c>IShape3D</c> (sphere or capsule) refreshed from ECS components.
 */
class DynamicCollider3D {
public:
    DynamicCollider3D() = default;
    DynamicCollider3D(DynamicCollider3D&&) noexcept = default;
    DynamicCollider3D& operator=(DynamicCollider3D&&) noexcept = default;
    DynamicCollider3D(const DynamicCollider3D&) = delete;
    DynamicCollider3D& operator=(const DynamicCollider3D&) = delete;

    static DynamicCollider3D FromLegacySnapshot(const DynamicCollider3DSim& snapshot);

    [[nodiscard]] DynamicCollider3DSim ToLegacySnapshot() const;

    void RefreshFromSphere(GameObject& owner, const SphereCollider3DComponent& collider) noexcept;
    void RefreshFromCapsule(GameObject& owner, const CapsuleCollider3DComponent& collider) noexcept;

    [[nodiscard]] bool IsValid() const noexcept { return shape != nullptr; }
    [[nodiscard]] const IShape3D& GetShape() const noexcept { return *shape; }
    [[nodiscard]] ShapeType3D GetShapeType() const noexcept;
    [[nodiscard]] DynamicCollider3DShape GetLegacyShapeTag() const noexcept;
    [[nodiscard]] CollisionAabb3 GetBounds() const noexcept;
    [[nodiscard]] Vector3 GetTrackingPoint() const noexcept;
    [[nodiscard]] float GetRepresentativeRadius() const noexcept;
    /** Radius used for static impulse torque (sphere radius or capsule end-cap radius, not bounding sphere). */
    [[nodiscard]] float GetImpulseContactRadius() const noexcept;

    void Translate(const Vector3& delta) noexcept;

    [[nodiscard]] bool OverlapsStatic(const Collider3D& staticCollider) const noexcept;
    [[nodiscard]] bool ComputeStaticContact(
            const Collider3D& staticCollider,
            float& outNormalX,
            float& outNormalY,
            float& outNormalZ,
            float& outPenetration,
            float separationSlop = 0.0F) const noexcept;
    [[nodiscard]] bool SeparateFromStatic(const Collider3D& staticCollider) noexcept;

    [[nodiscard]] bool Overlaps(const DynamicCollider3D& other) const noexcept;
    [[nodiscard]] bool ComputeDynamicContact(
            const DynamicCollider3D& other,
            float& outNormalX,
            float& outNormalY,
            float& outNormalZ,
            float& outPenetration) const noexcept;
    [[nodiscard]] bool SeparateFromDynamic(
            DynamicCollider3D& other,
            float inverseMassA,
            float inverseMassB) noexcept;

    [[nodiscard]] float EffectiveInverseInertia(const Rigidbody3DComponent& rb) const noexcept;
    void ComputeDynamicContactPointOnA(
            const DynamicCollider3D& other,
            float normalX,
            float normalY,
            float normalZ,
            Vector3& outPointOnA) const noexcept;

private:
    UniquePtr<IShape3D> shape;
};

// --- Legacy free-function API (delegates to <c>DynamicCollider3D</c> where applicable) ---

void BuildDynamicCollider3DSimFromSphere(
        GameObject& owner,
        const SphereCollider3DComponent& collider,
        DynamicCollider3DSim& out) noexcept;

void BuildDynamicCollider3DSimFromCapsule(
        GameObject& owner,
        const CapsuleCollider3DComponent& collider,
        DynamicCollider3DSim& out) noexcept;

[[nodiscard]] Vector3 GetDynamicCollider3DTrackingPoint(const DynamicCollider3DSim& sim) noexcept;
[[nodiscard]] Vector3 GetDynamicCollider3DTrackingPoint(const DynamicCollider3D& collider) noexcept;

void TranslateDynamicCollider3DSim(DynamicCollider3DSim& sim, const Vector3& delta) noexcept;
void RebuildDynamicCollider3DBounds(DynamicCollider3DSim& sim) noexcept;

[[nodiscard]] bool DynamicCollider3DOverlapsStatic(
        const DynamicCollider3DSim& dynamic, const StaticCollider3DSim& staticCollider) noexcept;
[[nodiscard]] bool DynamicCollider3DOverlapsStatic(
        const DynamicCollider3DSim& dynamic, const Collider3D& staticCollider) noexcept;
[[nodiscard]] bool DynamicCollider3DOverlapsStatic(
        const DynamicCollider3D& dynamic, const Collider3D& staticCollider) noexcept;

[[nodiscard]] bool ComputeDynamicStaticCollider3Contact(
        const DynamicCollider3DSim& dynamic,
        const StaticCollider3DSim& staticCollider,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;
[[nodiscard]] bool ComputeDynamicStaticCollider3Contact(
        const DynamicCollider3DSim& dynamic,
        const Collider3D& staticCollider,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;
[[nodiscard]] bool ComputeDynamicStaticCollider3Contact(
        const DynamicCollider3D& dynamic,
        const Collider3D& staticCollider,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        float separationSlop = 0.0F) noexcept;

[[nodiscard]] bool SeparateDynamicCollider3DFromStatic(
        DynamicCollider3DSim& dynamic, const StaticCollider3DSim& staticCollider) noexcept;
[[nodiscard]] bool SeparateDynamicCollider3DFromStatic(
        DynamicCollider3DSim& dynamic, const Collider3D& staticCollider) noexcept;
[[nodiscard]] bool SeparateDynamicCollider3DFromStatic(
        DynamicCollider3D& dynamic, const Collider3D& staticCollider) noexcept;

[[nodiscard]] bool ComputeDynamicDynamicCollider3Contact(
        const DynamicCollider3DSim& a,
        const DynamicCollider3DSim& b,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration) noexcept;
[[nodiscard]] bool ComputeDynamicDynamicCollider3Contact(
        const DynamicCollider3D& a,
        const DynamicCollider3D& b,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration) noexcept;

[[nodiscard]] bool SeparateDynamicDynamicCollider3Position(
        DynamicCollider3DSim& a,
        DynamicCollider3DSim& b,
        float inverseMassA,
        float inverseMassB) noexcept;
[[nodiscard]] bool SeparateDynamicDynamicCollider3Position(
        DynamicCollider3D& a,
        DynamicCollider3D& b,
        float inverseMassA,
        float inverseMassB) noexcept;

[[nodiscard]] float EffectiveDynamicCollider3DInverseInertia(
        const Rigidbody3DComponent& rb, const DynamicCollider3DSim& sim) noexcept;
[[nodiscard]] float EffectiveDynamicCollider3DInverseInertia(
        const Rigidbody3DComponent& rb, const DynamicCollider3D& collider) noexcept;

void ComputeDynamicDynamicContactPointOnA(
        const DynamicCollider3DSim& a,
        const DynamicCollider3DSim& b,
        float normalX,
        float normalY,
        float normalZ,
        Vector3& outPointOnA) noexcept;
void ComputeDynamicDynamicContactPointOnA(
        const DynamicCollider3D& a,
        const DynamicCollider3D& b,
        float normalX,
        float normalY,
        float normalZ,
        Vector3& outPointOnA) noexcept;

[[nodiscard]] bool TryBuildTriggerProbe3DFromObject(
        GameObject& object,
        const TriggerVolume3DSettings& settings,
        DynamicCollider3DSim& outProbe) noexcept;
[[nodiscard]] bool TryBuildTriggerProbe3DFromObject(
        GameObject& object,
        const TriggerVolume3DSettings& settings,
        DynamicCollider3D& outProbe) noexcept;

}  // namespace Spark
