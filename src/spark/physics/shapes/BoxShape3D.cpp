#include "spark/physics/shapes/BoxShape3D.hpp"

#include "spark/physics/Collision3D.hpp"
#include "spark/physics/shapes/ShapeContact3DDetail.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

bool BoxShape3D::Overlaps(const IShape3D& other) const {
    return ShapeContact3DDetail::OverlapPair(*this, other);
}

bool BoxShape3D::OverlapsAabb(const CollisionAabb3& otherAabb) const {
    return CollisionAabb3Overlaps(aabb, otherAabb);
}

bool BoxShape3D::OverlapsSphere(const Vector3& center, const float radius) const {
    return CollisionAabb3OverlapsSphere(aabb, center, radius);
}

bool BoxShape3D::Raycast(const Ray3D& ray, float& outDistance) const {
    constexpr float kEps = 1.0e-8F;
    float t0 = 0.0F;
    float t1 = ray.maxDistance;

    const auto clipAxis = [&](const float origin, const float dir, const float minB, const float maxB) -> bool {
        if (std::fabs(dir) < kEps) {
            return origin >= minB - kEps && origin <= maxB + kEps;
        }
        const float inv = 1.0F / dir;
        float ta = (minB - origin) * inv;
        float tb = (maxB - origin) * inv;
        if (ta > tb) {
            const float tmp = ta;
            ta = tb;
            tb = tmp;
        }
        t0 = (std::max)(t0, ta);
        t1 = (std::min)(t1, tb);
        return t0 <= t1 + kEps;
    };

    if (!clipAxis(ray.origin.x, ray.direction.x, aabb.minX, aabb.maxX)) {
        return false;
    }
    if (!clipAxis(ray.origin.y, ray.direction.y, aabb.minY, aabb.maxY)) {
        return false;
    }
    if (!clipAxis(ray.origin.z, ray.direction.z, aabb.minZ, aabb.maxZ)) {
        return false;
    }

    const float tHit = (std::max)(0.0F, t0);
    if (tHit > t1 + kEps || tHit > ray.maxDistance + kEps) {
        return false;
    }
    outDistance = tHit;
    return true;
}

bool BoxShape3D::ComputeContact(const IShape3D& other, ContactManifold3D& out) const {
    return ShapeContact3DDetail::ContactPair(*this, other, out);
}

}  // namespace Spark
