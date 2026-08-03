#include "spark/physics/shapes/NarrowPhase2D.hpp"

#include "spark/physics/shapes/ShapeContact2DDetail.hpp"

namespace Spark {

bool NarrowPhase2D::Overlap(const IShape2D& a, const IShape2D& b) {
    return ShapeContact2DDetail::OverlapPair(a, b);
}

bool NarrowPhase2D::ComputeContact(const IShape2D& a, const IShape2D& b, ContactManifold2D& out) {
    return ShapeContact2DDetail::ContactPair(a, b, out);
}

bool NarrowPhase2D::Raycast(const IShape2D& shape, const Ray2D& ray, float& outDistance) {
    return shape.Raycast(ray, outDistance);
}

}  // namespace Spark
