#include "spark/physics/shapes/CapsuleShape3D.hpp"

#include "spark/physics/Collision3D.hpp"
#include "spark/physics/shapes/ShapeContact3DDetail.hpp"

#include <algorithm>

namespace Spark {

CollisionAabb3 CapsuleShape3D::GetBounds() const noexcept {
    CollisionAabb3 bounds{};
    bounds.minX = (std::min)(capsule.pointA.x, capsule.pointB.x) - capsule.radius;
    bounds.maxX = (std::max)(capsule.pointA.x, capsule.pointB.x) + capsule.radius;
    bounds.minY = (std::min)(capsule.pointA.y, capsule.pointB.y) - capsule.radius;
    bounds.maxY = (std::max)(capsule.pointA.y, capsule.pointB.y) + capsule.radius;
    bounds.minZ = (std::min)(capsule.pointA.z, capsule.pointB.z) - capsule.radius;
    bounds.maxZ = (std::max)(capsule.pointA.z, capsule.pointB.z) + capsule.radius;
    return bounds;
}

void CapsuleShape3D::Translate(const Vector3& delta) noexcept {
    capsule.pointA.x += delta.x;
    capsule.pointA.y += delta.y;
    capsule.pointA.z += delta.z;
    capsule.pointB.x += delta.x;
    capsule.pointB.y += delta.y;
    capsule.pointB.z += delta.z;
}

bool CapsuleShape3D::Overlaps(const IShape3D& other) const {
    return ShapeContact3DDetail::OverlapPair(*this, other);
}

bool CapsuleShape3D::OverlapsAabb(const CollisionAabb3& aabb) const {
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;
    return ComputeCapsuleAabbContact(capsule, aabb, nx, ny, nz, pen);
}

bool CapsuleShape3D::OverlapsSphere(const Vector3& center, const float radius) const {
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;
    return ComputeSphereCapsuleContact(center, radius, capsule, nx, ny, nz, pen);
}

bool CapsuleShape3D::Raycast(const Ray3D& ray, float& outDistance) const {
    (void)ray;
    (void)outDistance;
    return false;
}

bool CapsuleShape3D::ComputeContact(const IShape3D& other, ContactManifold3D& out) const {
    return ShapeContact3DDetail::ContactPair(*this, other, out);
}

}  // namespace Spark
