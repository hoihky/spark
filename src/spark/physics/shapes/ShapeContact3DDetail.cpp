#include "spark/physics/shapes/ShapeContact3DDetail.hpp"

#include "spark/physics/Collision3D.hpp"
#include "spark/physics/shapes/BoxShape3D.hpp"
#include "spark/physics/shapes/CapsuleShape3D.hpp"
#include "spark/physics/shapes/ShapeType3D.hpp"
#include "spark/physics/shapes/SphereShape3D.hpp"

#include "spark/physics/shapes/ShapeType3D.hpp"

#include <algorithm>
#include <cmath>

namespace Spark::ShapeContact3DDetail {

namespace {

[[nodiscard]] bool FillManifold(
        const float nx,
        const float ny,
        const float nz,
        const float penetration,
        ContactManifold3D& out) noexcept {
    if (penetration <= 0.0F) {
        return false;
    }
    out.normal = {nx, ny, nz};
    out.point = {0.0F, 0.0F, 0.0F};
    out.penetration = penetration;
    return true;
}

[[nodiscard]] const BoxShape3D* AsBox(const IShape3D& shape) noexcept {
    return shape.GetType() == ShapeType3D::Box ? static_cast<const BoxShape3D*>(&shape) : nullptr;
}

[[nodiscard]] const SphereShape3D* AsSphere(const IShape3D& shape) noexcept {
    return shape.GetType() == ShapeType3D::Sphere ? static_cast<const SphereShape3D*>(&shape) : nullptr;
}

[[nodiscard]] const CapsuleShape3D* AsCapsule(const IShape3D& shape) noexcept {
    return shape.GetType() == ShapeType3D::Capsule ? static_cast<const CapsuleShape3D*>(&shape) : nullptr;
}

[[nodiscard]] bool SphereSphereOverlap(const SphereShape3D& a, const SphereShape3D& b) noexcept {
    const Vector3 delta = b.GetCenter() - a.GetCenter();
    const float sum = a.GetRadius() + b.GetRadius();
    return delta.LengthSquared() <= sum * sum + 1.0e-8F;
}

}  // namespace

bool OverlapPair(const IShape3D& a, const IShape3D& b) noexcept {
    if (const BoxShape3D* boxA = AsBox(a)) {
        if (const BoxShape3D* boxB = AsBox(b)) {
            return CollisionAabb3Overlaps(boxA->GetAabb(), boxB->GetAabb());
        }
        if (const SphereShape3D* sphereB = AsSphere(b)) {
            return CollisionAabb3OverlapsSphere(boxA->GetAabb(), sphereB->GetCenter(), sphereB->GetRadius());
        }
        if (const CapsuleShape3D* capsuleB = AsCapsule(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            return ComputeCapsuleAabbContact(capsuleB->GetCapsule(), boxA->GetAabb(), nx, ny, nz, pen);
        }
    }
    if (const SphereShape3D* sphereA = AsSphere(a)) {
        if (const BoxShape3D* boxB = AsBox(b)) {
            return CollisionAabb3OverlapsSphere(boxB->GetAabb(), sphereA->GetCenter(), sphereA->GetRadius());
        }
        if (const SphereShape3D* sphereB = AsSphere(b)) {
            return SphereSphereOverlap(*sphereA, *sphereB);
        }
        if (const CapsuleShape3D* capsuleB = AsCapsule(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            return ComputeSphereCapsuleContact(
                    sphereA->GetCenter(), sphereA->GetRadius(), capsuleB->GetCapsule(), nx, ny, nz, pen);
        }
    }
    if (const CapsuleShape3D* capsuleA = AsCapsule(a)) {
        if (const BoxShape3D* boxB = AsBox(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            return ComputeCapsuleAabbContact(capsuleA->GetCapsule(), boxB->GetAabb(), nx, ny, nz, pen);
        }
        if (const SphereShape3D* sphereB = AsSphere(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            return ComputeSphereCapsuleContact(
                    sphereB->GetCenter(), sphereB->GetRadius(), capsuleA->GetCapsule(), nx, ny, nz, pen);
        }
        if (const CapsuleShape3D* capsuleB = AsCapsule(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            return ComputeCapsuleCapsuleContact(capsuleA->GetCapsule(), capsuleB->GetCapsule(), nx, ny, nz, pen);
        }
    }
    return false;
}

bool ContactPair(const IShape3D& a, const IShape3D& b, ContactManifold3D& out) noexcept {
    out.Clear();
    if (const SphereShape3D* sphereA = AsSphere(a)) {
        if (const BoxShape3D* boxB = AsBox(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            if (!ComputeSphereAabbContact(sphereA->GetCenter(), sphereA->GetRadius(), boxB->GetAabb(), nx, ny, nz, pen)) {
                return false;
            }
            return FillManifold(nx, ny, nz, pen, out);
        }
        if (const SphereShape3D* sphereB = AsSphere(b)) {
            const Vector3 delta = sphereB->GetCenter() - sphereA->GetCenter();
            const float sum = sphereA->GetRadius() + sphereB->GetRadius();
            const float distSq = delta.LengthSquared();
            if (distSq > sum * sum + 1.0e-8F) {
                return false;
            }
            if (distSq <= 1.0e-12F) {
                return FillManifold(0.0F, 1.0F, 0.0F, sum, out);
            }
            const float dist = std::sqrt(distSq);
            return FillManifold(delta.x / dist, delta.y / dist, delta.z / dist, sum - dist, out);
        }
        if (const CapsuleShape3D* capsuleB = AsCapsule(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            if (!ComputeSphereCapsuleContact(
                        sphereA->GetCenter(), sphereA->GetRadius(), capsuleB->GetCapsule(), nx, ny, nz, pen)) {
                return false;
            }
            return FillManifold(nx, ny, nz, pen, out);
        }
    }
    if (const CapsuleShape3D* capsuleA = AsCapsule(a)) {
        if (const BoxShape3D* boxB = AsBox(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            if (!ComputeCapsuleAabbContact(capsuleA->GetCapsule(), boxB->GetAabb(), nx, ny, nz, pen)) {
                return false;
            }
            return FillManifold(nx, ny, nz, pen, out);
        }
        if (const SphereShape3D* sphereB = AsSphere(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            if (!ComputeSphereCapsuleContact(
                        sphereB->GetCenter(), sphereB->GetRadius(), capsuleA->GetCapsule(), nx, ny, nz, pen)) {
                return false;
            }
            out.normal = {-nx, -ny, -nz};
            out.point = sphereB->GetCenter();
            out.penetration = pen;
            return true;
        }
        if (const CapsuleShape3D* capsuleB = AsCapsule(b)) {
            float nx = 0.0F;
            float ny = 0.0F;
            float nz = 0.0F;
            float pen = 0.0F;
            if (!ComputeCapsuleCapsuleContact(capsuleA->GetCapsule(), capsuleB->GetCapsule(), nx, ny, nz, pen)) {
                return false;
            }
            return FillManifold(nx, ny, nz, pen, out);
        }
    }
    if (const BoxShape3D* boxA = AsBox(a)) {
        if (const BoxShape3D* boxB = AsBox(b)) {
            if (!CollisionAabb3Overlaps(boxA->GetAabb(), boxB->GetAabb())) {
                return false;
            }
            const CollisionAabb3& aabbA = boxA->GetAabb();
            const CollisionAabb3& aabbB = boxB->GetAabb();
            const float overlapX = (std::min)(aabbA.maxX, aabbB.maxX) - (std::max)(aabbA.minX, aabbB.minX);
            const float overlapY = (std::min)(aabbA.maxY, aabbB.maxY) - (std::max)(aabbA.minY, aabbB.minY);
            const float overlapZ = (std::min)(aabbA.maxZ, aabbB.maxZ) - (std::max)(aabbA.minZ, aabbB.minZ);
            if (overlapX <= overlapY && overlapX <= overlapZ) {
                const float centerAx = 0.5F * (aabbA.minX + aabbA.maxX);
                const float centerBx = 0.5F * (aabbB.minX + aabbB.maxX);
                return FillManifold(centerAx < centerBx ? -1.0F : 1.0F, 0.0F, 0.0F, overlapX, out);
            }
            if (overlapY <= overlapZ) {
                const float centerAy = 0.5F * (aabbA.minY + aabbA.maxY);
                const float centerBy = 0.5F * (aabbB.minY + aabbB.maxY);
                return FillManifold(0.0F, centerAy < centerBy ? -1.0F : 1.0F, 0.0F, overlapY, out);
            }
            const float centerAz = 0.5F * (aabbA.minZ + aabbA.maxZ);
            const float centerBz = 0.5F * (aabbB.minZ + aabbB.maxZ);
            return FillManifold(0.0F, 0.0F, centerAz < centerBz ? -1.0F : 1.0F, overlapZ, out);
        }
    }
    return false;
}

}  // namespace Spark::ShapeContact3DDetail
