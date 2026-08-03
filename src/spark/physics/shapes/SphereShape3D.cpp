#include "spark/physics/shapes/SphereShape3D.hpp"

#include "spark/physics/Collision3D.hpp"
#include "spark/physics/shapes/ShapeContact3DDetail.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Spark {

CollisionAabb3 SphereShape3D::GetBounds() const noexcept {
    CollisionAabb3 bounds{};
    bounds.minX = center.x - radius;
    bounds.maxX = center.x + radius;
    bounds.minY = center.y - radius;
    bounds.maxY = center.y + radius;
    bounds.minZ = center.z - radius;
    bounds.maxZ = center.z + radius;
    return bounds;
}

void SphereShape3D::Translate(const Vector3& delta) noexcept {
    center.x += delta.x;
    center.y += delta.y;
    center.z += delta.z;
}

bool SphereShape3D::Overlaps(const IShape3D& other) const {
    return ShapeContact3DDetail::OverlapPair(*this, other);
}

bool SphereShape3D::OverlapsAabb(const CollisionAabb3& aabb) const {
    return CollisionAabb3OverlapsSphere(aabb, center, radius);
}

bool SphereShape3D::OverlapsSphere(const Vector3& otherCenter, const float otherRadius) const {
    const Vector3 delta = otherCenter - center;
    const float sum = radius + otherRadius;
    return delta.LengthSquared() <= sum * sum + 1.0e-8F;
}

bool SphereShape3D::Raycast(const Ray3D& ray, float& outDistance) const {
    constexpr float kEps = 1.0e-8F;
    const float lx = ray.origin.x - center.x;
    const float ly = ray.origin.y - center.y;
    const float lz = ray.origin.z - center.z;
    const float dx = ray.direction.x;
    const float dy = ray.direction.y;
    const float dz = ray.direction.z;
    const float a = dx * dx + dy * dy + dz * dz;
    if (a < kEps) {
        return false;
    }
    const float b = 2.0F * (dx * lx + dy * ly + dz * lz);
    const float c = lx * lx + ly * ly + lz * lz - radius * radius;
    const float disc = b * b - 4.0F * a * c;
    if (disc < 0.0F) {
        return false;
    }
    const float sd = std::sqrt(disc);
    const float inv2a = 1.0F / (2.0F * a);
    const float tA = (-b - sd) * inv2a;
    const float tB = (-b + sd) * inv2a;

    float best = std::numeric_limits<float>::infinity();
    auto consider = [&](const float t) noexcept {
        if (t >= -kEps && t <= ray.maxDistance + kEps) {
            const float tt = (std::max)(0.0F, t);
            if (tt < best) {
                best = tt;
            }
        }
    };
    consider(tA);
    consider(tB);
    if (!std::isfinite(best)) {
        return false;
    }
    outDistance = best;
    return true;
}

bool SphereShape3D::ComputeContact(const IShape3D& other, ContactManifold3D& out) const {
    return ShapeContact3DDetail::ContactPair(*this, other, out);
}

}  // namespace Spark
