#include "spark/physics/shapes/NarrowPhase3D.hpp"

#include "spark/physics/shapes/ShapeContact3DDetail.hpp"

namespace Spark {

bool NarrowPhase3D::Overlap(const IShape3D& a, const IShape3D& b) {
    return ShapeContact3DDetail::OverlapPair(a, b);
}

bool NarrowPhase3D::ComputeContact(const IShape3D& a, const IShape3D& b, ContactManifold3D& out) {
    return ShapeContact3DDetail::ContactPair(a, b, out);
}

bool NarrowPhase3D::Raycast(const IShape3D& shape, const Ray3D& ray, float& outDistance) {
    return shape.Raycast(ray, outDistance);
}

}  // namespace Spark
