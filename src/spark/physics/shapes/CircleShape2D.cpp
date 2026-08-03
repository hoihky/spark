#include "spark/physics/shapes/CircleShape2D.hpp"

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/shapes/ShapeContact2DDetail.hpp"

#include <algorithm>

namespace Spark {

CollisionAabb2 CircleShape2D::GetBounds() const noexcept {
    CollisionAabb2 bounds{};
    bounds.minX = centerX - radius;
    bounds.maxX = centerX + radius;
    bounds.minY = centerY - radius;
    bounds.maxY = centerY + radius;
    return bounds;
}

void CircleShape2D::Translate(const float deltaX, const float deltaY) noexcept {
    centerX += deltaX;
    centerY += deltaY;
}

bool CircleShape2D::Overlaps(const IShape2D& other) const {
    return ShapeContact2DDetail::OverlapPair(*this, other);
}

bool CircleShape2D::OverlapsAabb(const CollisionAabb2& aabb) const {
    return CollisionAabb2OverlapsCircle(aabb, centerX, centerY, radius);
}

bool CircleShape2D::OverlapsCircle(
        const float otherCenterX,
        const float otherCenterY,
        const float otherRadius) const {
    return CollisionCirclesOverlap(centerX, centerY, radius, otherCenterX, otherCenterY, otherRadius);
}

bool CircleShape2D::Raycast(const Ray2D& ray, float& outDistance) const {
    return RaycastSegmentCircle2(
            ray.origin.x,
            ray.origin.y,
            ray.direction.x,
            ray.direction.y,
            ray.maxDistance,
            centerX,
            centerY,
            radius,
            outDistance);
}

bool CircleShape2D::ComputeContact(const IShape2D& other, ContactManifold2D& out) const {
    return ShapeContact2DDetail::ContactPair(*this, other, out);
}

}  // namespace Spark
