#include "spark/physics/DynamicCollider3D.hpp"

#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/CollisionComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/physics/TriggerVolume3D.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

void BuildCapsuleBounds(const CollisionCapsule3& capsule, CollisionAabb3& outBounds) noexcept {
    outBounds.minX = std::min(capsule.pointA.x, capsule.pointB.x) - capsule.radius;
    outBounds.minY = std::min(capsule.pointA.y, capsule.pointB.y) - capsule.radius;
    outBounds.minZ = std::min(capsule.pointA.z, capsule.pointB.z) - capsule.radius;
    outBounds.maxX = std::max(capsule.pointA.x, capsule.pointB.x) + capsule.radius;
    outBounds.maxY = std::max(capsule.pointA.y, capsule.pointB.y) + capsule.radius;
    outBounds.maxZ = std::max(capsule.pointA.z, capsule.pointB.z) + capsule.radius;
}

void BuildSphereBounds(const Vector3& center, const float radius, CollisionAabb3& outBounds) noexcept {
    outBounds.minX = center.x - radius;
    outBounds.minY = center.y - radius;
    outBounds.minZ = center.z - radius;
    outBounds.maxX = center.x + radius;
    outBounds.maxY = center.y + radius;
    outBounds.maxZ = center.z + radius;
}

[[nodiscard]] Vector3 CapsuleMidpoint(const CollisionCapsule3& capsule) noexcept {
    return {
            0.5F * (capsule.pointA.x + capsule.pointB.x),
            0.5F * (capsule.pointA.y + capsule.pointB.y),
            0.5F * (capsule.pointA.z + capsule.pointB.z)};
}

[[nodiscard]] float CapsuleBoundingSphereRadius(const CollisionCapsule3& capsule) noexcept {
    const float hx = capsule.pointB.x - capsule.pointA.x;
    const float hy = capsule.pointB.y - capsule.pointA.y;
    const float hz = capsule.pointB.z - capsule.pointA.z;
    const float halfLen = 0.5F * std::sqrt(hx * hx + hy * hy + hz * hz);
    return capsule.radius + halfLen;
}

[[nodiscard]] float SolidSphereInverseInertiaScalar(const float invMass, const float radius) noexcept {
    if (invMass <= 0.0F || radius < 1.0e-6F) {
        return 0.0F;
    }
    const float rr = radius * radius;
    return 2.5F * invMass / rr;
}

void BuildCharacterControllerProbeSphere(
        GameObject& owner,
        const CharacterController3DComponent& controller,
        DynamicCollider3DSim& out) noexcept {
    const Matrix4 worldMatrix = owner.GetWorldMatrix();
    const Vector3 localCenter = controller.GetCenterOffset();
    const Vector4 pc = worldMatrix * Vector4(localCenter.x, localCenter.y, localCenter.z, 1.0F);
    const float w = (std::fabs(pc.w) < 1.0e-8F) ? 1.0F : pc.w;
    out.shape = DynamicCollider3DShape::Sphere;
    out.sphereCenter = {pc.x / w, pc.y / w, pc.z / w};
    const float sx = std::sqrt(
            worldMatrix.m[0] * worldMatrix.m[0] + worldMatrix.m[1] * worldMatrix.m[1] +
            worldMatrix.m[2] * worldMatrix.m[2]);
    const float sy = std::sqrt(
            worldMatrix.m[4] * worldMatrix.m[4] + worldMatrix.m[5] * worldMatrix.m[5] +
            worldMatrix.m[6] * worldMatrix.m[6]);
    const float sz = std::sqrt(
            worldMatrix.m[8] * worldMatrix.m[8] + worldMatrix.m[9] * worldMatrix.m[9] +
            worldMatrix.m[10] * worldMatrix.m[10]);
    const float scale = std::max({sx, sy, sz});
    out.sphereRadius = controller.GetRadius() * scale;
    RebuildDynamicCollider3DBounds(out);
}

void BuildCollisionComponentProbeSphere(
        GameObject& owner,
        CollisionComponent& collision,
        DynamicCollider3DSim& out) noexcept {
    collision.RefreshWorldBounds(owner);
    out.shape = DynamicCollider3DShape::Sphere;
    out.sphereCenter = collision.GetWorldCenter();
    out.sphereRadius = collision.GetRadius();
    RebuildDynamicCollider3DBounds(out);
}

}  // namespace

void BuildDynamicCollider3DSimFromSphere(
        GameObject& owner,
        const SphereCollider3DComponent& collider,
        DynamicCollider3DSim& out) noexcept {
    out.shape = DynamicCollider3DShape::Sphere;
    ComputeSphereCollider3World(owner, collider, out.sphereCenter, out.sphereRadius);
    BuildSphereBounds(out.sphereCenter, out.sphereRadius, out.bounds);
}

void BuildDynamicCollider3DSimFromCapsule(
        GameObject& owner,
        const CapsuleCollider3DComponent& collider,
        DynamicCollider3DSim& out) noexcept {
    out.shape = DynamicCollider3DShape::Capsule;
    ComputeCapsuleCollider3World(owner, collider, out.capsule);
    BuildCapsuleBounds(out.capsule, out.bounds);
}

Vector3 GetDynamicCollider3DTrackingPoint(const DynamicCollider3DSim& sim) noexcept {
    if (sim.shape == DynamicCollider3DShape::Sphere) {
        return sim.sphereCenter;
    }
    return CapsuleMidpoint(sim.capsule);
}

void TranslateDynamicCollider3DSim(DynamicCollider3DSim& sim, const Vector3& delta) noexcept {
    if (sim.shape == DynamicCollider3DShape::Sphere) {
        sim.sphereCenter.x += delta.x;
        sim.sphereCenter.y += delta.y;
        sim.sphereCenter.z += delta.z;
    } else {
        sim.capsule.pointA.x += delta.x;
        sim.capsule.pointA.y += delta.y;
        sim.capsule.pointA.z += delta.z;
        sim.capsule.pointB.x += delta.x;
        sim.capsule.pointB.y += delta.y;
        sim.capsule.pointB.z += delta.z;
    }
    RebuildDynamicCollider3DBounds(sim);
}

void RebuildDynamicCollider3DBounds(DynamicCollider3DSim& sim) noexcept {
    if (sim.shape == DynamicCollider3DShape::Sphere) {
        BuildSphereBounds(sim.sphereCenter, sim.sphereRadius, sim.bounds);
    } else {
        BuildCapsuleBounds(sim.capsule, sim.bounds);
    }
}

bool DynamicCollider3DOverlapsStatic(
        const DynamicCollider3DSim& dynamic,
        const StaticCollider3DSim& staticCollider) noexcept {
    if (dynamic.shape == DynamicCollider3DShape::Sphere) {
        return StaticCollider3DOverlapsSphere(staticCollider, dynamic.sphereCenter, dynamic.sphereRadius);
    }

    if (!CollisionAabb3Overlaps(dynamic.bounds, staticCollider.aabb)) {
        return false;
    }
    if (staticCollider.shape == StaticCollider3DShape::Capsule) {
        float nx = 0.0F;
        float ny = 0.0F;
        float nz = 0.0F;
        float pen = 0.0F;
        return ComputeCapsuleCapsuleContact(dynamic.capsule, staticCollider.capsule, nx, ny, nz, pen);
    }
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;
    return ComputeCapsuleAabbContact(dynamic.capsule, staticCollider.aabb, nx, ny, nz, pen);
}

bool ComputeDynamicStaticCollider3Contact(
        const DynamicCollider3DSim& dynamic,
        const StaticCollider3DSim& staticCollider,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration,
        const float separationSlop) noexcept {
    if (dynamic.shape == DynamicCollider3DShape::Sphere) {
        return ComputeSphereStaticCollider3Contact(
                dynamic.sphereCenter,
                dynamic.sphereRadius,
                staticCollider,
                outNormalX,
                outNormalY,
                outNormalZ,
                outPenetration,
                separationSlop);
    }
    if (staticCollider.shape == StaticCollider3DShape::Capsule) {
        return ComputeCapsuleCapsuleContact(
                dynamic.capsule,
                staticCollider.capsule,
                outNormalX,
                outNormalY,
                outNormalZ,
                outPenetration,
                separationSlop);
    }
    return ComputeCapsuleAabbContact(
            dynamic.capsule,
            staticCollider.aabb,
            outNormalX,
            outNormalY,
            outNormalZ,
            outPenetration,
            separationSlop);
}

bool SeparateDynamicCollider3DFromStatic(
        DynamicCollider3DSim& dynamic,
        const StaticCollider3DSim& staticCollider) noexcept {
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;
    if (!ComputeDynamicStaticCollider3Contact(dynamic, staticCollider, nx, ny, nz, pen)) {
        return false;
    }
    if (pen > 1.0e-8F) {
        TranslateDynamicCollider3DSim(dynamic, {nx * pen, ny * pen, nz * pen});
    }
    return true;
}

bool ComputeDynamicDynamicCollider3Contact(
        const DynamicCollider3DSim& a,
        const DynamicCollider3DSim& b,
        float& outNormalX,
        float& outNormalY,
        float& outNormalZ,
        float& outPenetration) noexcept {
    if (a.shape == DynamicCollider3DShape::Sphere && b.shape == DynamicCollider3DShape::Sphere) {
        const Vector3 d{
                b.sphereCenter.x - a.sphereCenter.x,
                b.sphereCenter.y - a.sphereCenter.y,
                b.sphereCenter.z - a.sphereCenter.z};
        const float distSq = d.x * d.x + d.y * d.y + d.z * d.z;
        if (distSq < 1.0e-16F) {
            return false;
        }
        const float dist = std::sqrt(distSq);
        const float pen = a.sphereRadius + b.sphereRadius - dist;
        if (pen <= 0.0F) {
            return false;
        }
        outNormalX = d.x / dist;
        outNormalY = d.y / dist;
        outNormalZ = d.z / dist;
        outPenetration = pen;
        return true;
    }

    if (a.shape == DynamicCollider3DShape::Sphere && b.shape == DynamicCollider3DShape::Capsule) {
        float nx = 0.0F;
        float ny = 0.0F;
        float nz = 0.0F;
        float pen = 0.0F;
        if (!ComputeSphereCapsuleContact(a.sphereCenter, a.sphereRadius, b.capsule, nx, ny, nz, pen)) {
            return false;
        }
        outNormalX = -nx;
        outNormalY = -ny;
        outNormalZ = -nz;
        outPenetration = pen;
        return true;
    }

    if (a.shape == DynamicCollider3DShape::Capsule && b.shape == DynamicCollider3DShape::Sphere) {
        float nx = 0.0F;
        float ny = 0.0F;
        float nz = 0.0F;
        float pen = 0.0F;
        if (!ComputeSphereCapsuleContact(b.sphereCenter, b.sphereRadius, a.capsule, nx, ny, nz, pen)) {
            return false;
        }
        outNormalX = nx;
        outNormalY = ny;
        outNormalZ = nz;
        outPenetration = pen;
        return true;
    }

    return ComputeCapsuleCapsuleContact(a.capsule, b.capsule, outNormalX, outNormalY, outNormalZ, outPenetration);
}

bool SeparateDynamicDynamicCollider3Position(
        DynamicCollider3DSim& a,
        DynamicCollider3DSim& b,
        const float inverseMassA,
        const float inverseMassB) noexcept {
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;
    if (!ComputeDynamicDynamicCollider3Contact(a, b, nx, ny, nz, pen) || pen <= 0.0F) {
        return false;
    }

    const float den = inverseMassA + inverseMassB;
    if (den < 1.0e-10F) {
        return false;
    }
    const float corr = pen / den;
    TranslateDynamicCollider3DSim(a, {-nx * corr * inverseMassB, -ny * corr * inverseMassB, -nz * corr * inverseMassB});
    TranslateDynamicCollider3DSim(b, {nx * corr * inverseMassA, ny * corr * inverseMassA, nz * corr * inverseMassA});
    return true;
}

float EffectiveDynamicCollider3DInverseInertia(
        const Rigidbody3DComponent& rb,
        const DynamicCollider3DSim& sim) noexcept {
    const float overrideInv = rb.GetInverseInertiaTensorScale();
    if (overrideInv > 0.0F) {
        return overrideInv;
    }

    const float invMass = rb.GetInverseMass();
    if (sim.shape == DynamicCollider3DShape::Sphere) {
        return SolidSphereInverseInertiaScalar(invMass, sim.sphereRadius);
    }
    return SolidSphereInverseInertiaScalar(invMass, CapsuleBoundingSphereRadius(sim.capsule));
}

void ComputeDynamicDynamicContactPointOnA(
        const DynamicCollider3DSim& a,
        const DynamicCollider3DSim& b,
        const float normalX,
        const float normalY,
        const float normalZ,
        Vector3& outPointOnA) noexcept {
    if (a.shape == DynamicCollider3DShape::Sphere) {
        outPointOnA = {
                a.sphereCenter.x + normalX * a.sphereRadius,
                a.sphereCenter.y + normalY * a.sphereRadius,
                a.sphereCenter.z + normalZ * a.sphereRadius};
        return;
    }

    const Vector3 mid = CapsuleMidpoint(a.capsule);
    if (b.shape == DynamicCollider3DShape::Sphere) {
        Vector3 closest{};
        float t = 0.0F;
        const float abx = a.capsule.pointB.x - a.capsule.pointA.x;
        const float aby = a.capsule.pointB.y - a.capsule.pointA.y;
        const float abz = a.capsule.pointB.z - a.capsule.pointA.z;
        const float abLenSq = abx * abx + aby * aby + abz * abz;
        if (abLenSq < 1.0e-12F) {
            outPointOnA = {
                    mid.x - normalX * a.capsule.radius,
                    mid.y - normalY * a.capsule.radius,
                    mid.z - normalZ * a.capsule.radius};
            return;
        }
        const float apx = b.sphereCenter.x - a.capsule.pointA.x;
        const float apy = b.sphereCenter.y - a.capsule.pointA.y;
        const float apz = b.sphereCenter.z - a.capsule.pointA.z;
        t = std::clamp((apx * abx + apy * aby + apz * abz) / abLenSq, 0.0F, 1.0F);
        closest.x = a.capsule.pointA.x + abx * t;
        closest.y = a.capsule.pointA.y + aby * t;
        closest.z = a.capsule.pointA.z + abz * t;
        outPointOnA = {
                closest.x - normalX * a.capsule.radius,
                closest.y - normalY * a.capsule.radius,
                closest.z - normalZ * a.capsule.radius};
        return;
    }

    Vector3 c1{};
    Vector3 c2{};
    const float d1x = a.capsule.pointB.x - a.capsule.pointA.x;
    const float d1y = a.capsule.pointB.y - a.capsule.pointA.y;
    const float d1z = a.capsule.pointB.z - a.capsule.pointA.z;
    const float d2x = b.capsule.pointB.x - b.capsule.pointA.x;
    const float d2y = b.capsule.pointB.y - b.capsule.pointA.y;
    const float d2z = b.capsule.pointB.z - b.capsule.pointA.z;
    const float rx = a.capsule.pointA.x - b.capsule.pointA.x;
    const float ry = a.capsule.pointA.y - b.capsule.pointA.y;
    const float rz = a.capsule.pointA.z - b.capsule.pointA.z;
    const float aa = d1x * d1x + d1y * d1y + d1z * d1z;
    const float ee = d2x * d2x + d2y * d2y + d2z * d2z;
    const float ff = d2x * rx + d2y * ry + d2z * rz;
    float s = 0.0F;
    float t = 0.0F;
    if (aa <= 1.0e-12F) {
        c1 = a.capsule.pointA;
    } else {
        const float cc = d1x * rx + d1y * ry + d1z * rz;
        if (ee <= 1.0e-12F) {
            s = std::clamp(-cc / aa, 0.0F, 1.0F);
        } else {
            const float bb = d1x * d2x + d1y * d2y + d1z * d2z;
            const float denom = aa * ee - bb * bb;
            s = (denom > 1.0e-12F) ? std::clamp((bb * ff - cc * ee) / denom, 0.0F, 1.0F) : 0.0F;
            t = (bb * s + ff) / ee;
            if (t < 0.0F) {
                t = 0.0F;
                s = std::clamp(-cc / aa, 0.0F, 1.0F);
            } else if (t > 1.0F) {
                t = 1.0F;
                s = std::clamp((bb - cc) / aa, 0.0F, 1.0F);
            }
        }
        c1.x = a.capsule.pointA.x + d1x * s;
        c1.y = a.capsule.pointA.y + d1y * s;
        c1.z = a.capsule.pointA.z + d1z * s;
    }
    (void)c2;
    (void)t;
    outPointOnA = {
            c1.x - normalX * a.capsule.radius,
            c1.y - normalY * a.capsule.radius,
            c1.z - normalZ * a.capsule.radius};
}

bool TryBuildTriggerProbe3DFromObject(
        GameObject& object,
        const TriggerVolume3DSettings& settings,
        DynamicCollider3DSim& outProbe) noexcept {
    if (settings.includeCharacterControllers) {
        if (auto* controller = object.GetComponent<CharacterController3DComponent>()) {
            BuildCharacterControllerProbeSphere(object, *controller, outProbe);
            return true;
        }
    }

    auto* rb = object.GetComponent<Rigidbody3DComponent>();
    const bool isDynamicRb = rb != nullptr && rb->GetBodyType() == RigidbodyBodyType3D::Dynamic;

    if (settings.includeDynamicRigidbodies && isDynamicRb) {
        if (auto* sphere = object.GetComponent<SphereCollider3DComponent>()) {
            BuildDynamicCollider3DSimFromSphere(object, *sphere, outProbe);
            return true;
        }
        if (auto* capsule = object.GetComponent<CapsuleCollider3DComponent>()) {
            BuildDynamicCollider3DSimFromCapsule(object, *capsule, outProbe);
            return true;
        }
    }

    if (settings.includeColliderWithoutRigidbody && !isDynamicRb) {
        if (auto* sphere = object.GetComponent<SphereCollider3DComponent>()) {
            BuildDynamicCollider3DSimFromSphere(object, *sphere, outProbe);
            return true;
        }
        if (auto* capsule = object.GetComponent<CapsuleCollider3DComponent>()) {
            BuildDynamicCollider3DSimFromCapsule(object, *capsule, outProbe);
            return true;
        }
    }

    if (settings.includeCollisionComponent) {
        if (auto* collision = object.GetComponent<CollisionComponent>()) {
            BuildCollisionComponentProbeSphere(object, *collision, outProbe);
            return true;
        }
    }

    return false;
}

}  // namespace Spark
