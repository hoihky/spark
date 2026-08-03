#include "spark/physics/shapes/ConvexPolygonShape2D.hpp"

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/shapes/ShapeContact2DDetail.hpp"

namespace Spark {

ConvexPolygonShape2D::ConvexPolygonShape2D(const StaticCollider2D& bakedPolygon) {
    snapshot = bakedPolygon;
    snapshot.shape = StaticCollider2DShape::ConvexPolygon;
    vertexCount = bakedPolygon.polygonVertexCount;
    bounds = bakedPolygon.aabb;
}

bool ConvexPolygonShape2D::Overlaps(const IShape2D& other) const {
    return ShapeContact2DDetail::OverlapPair(*this, other);
}

bool ConvexPolygonShape2D::OverlapsAabb(const CollisionAabb2& aabb) const {
    return CollisionConvexPolygonOverlapsWorldAabb(snapshot, aabb);
}

bool ConvexPolygonShape2D::OverlapsCircle(const float centerX, const float centerY, const float radius) const {
    return CollisionConvexPolygonOverlapsWorldCircle(snapshot, centerX, centerY, radius);
}

bool ConvexPolygonShape2D::Raycast(const Ray2D& ray, float& outDistance) const {
    (void)ray;
    (void)outDistance;
    return false;
}

bool ConvexPolygonShape2D::ComputeContact(const IShape2D& other, ContactManifold2D& out) const {
    return ShapeContact2DDetail::ContactPair(*this, other, out);
}

}  // namespace Spark
