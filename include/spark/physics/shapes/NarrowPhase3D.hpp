#pragma once

#include "spark/physics/core/ContactManifold.hpp"
#include "spark/physics/core/Ray.hpp"
#include "spark/physics/shapes/IShape3D.hpp"

namespace Spark {

/** Pairwise narrow-phase dispatch for <c>IShape3D</c>. */
class NarrowPhase3D {
public:
    [[nodiscard]] static bool Overlap(const IShape3D& a, const IShape3D& b);
    [[nodiscard]] static bool ComputeContact(const IShape3D& a, const IShape3D& b, ContactManifold3D& out);
    [[nodiscard]] static bool Raycast(const IShape3D& shape, const Ray3D& ray, float& outDistance);
};

}  // namespace Spark
