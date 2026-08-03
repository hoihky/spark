#include "spark/physics/shapes/BoxShape2D.hpp"

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/shapes/NarrowPhase2D.hpp"
#include "spark/physics/shapes/ShapeContact2DDetail.hpp"

namespace Spark {

void BoxShape2D::Translate(const float deltaX, const float deltaY) noexcept {
    aabb.minX += deltaX;
    aabb.maxX += deltaX;
    aabb.minY += deltaY;
    aabb.maxY += deltaY;
}

bool BoxShape2D::Overlaps(const IShape2D& other) const {
    return ShapeContact2DDetail::OverlapPair(*this, other);
}

bool BoxShape2D::OverlapsAabb(const CollisionAabb2& otherAabb) const {
    return CollisionAabb2Overlaps(aabb, otherAabb);
}

bool BoxShape2D::OverlapsCircle(const float centerX, const float centerY, const float radius) const {
    return CollisionAabb2OverlapsCircle(aabb, centerX, centerY, radius);
}

bool BoxShape2D::Raycast(const Ray2D& ray, float& outDistance) const {
    return RaycastSegmentAabb2(
            ray.origin.x, ray.origin.y, ray.direction.x, ray.direction.y, ray.maxDistance, aabb, outDistance);
}

bool BoxShape2D::ComputeContact(const IShape2D& other, ContactManifold2D& out) const {
    return ShapeContact2DDetail::ContactPair(*this, other, out);
}

}  // namespace Spark
