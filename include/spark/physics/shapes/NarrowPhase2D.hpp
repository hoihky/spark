#pragma once

#include "spark/physics/core/ContactManifold.hpp"
#include "spark/physics/core/Ray.hpp"
#include "spark/physics/shapes/IShape2D.hpp"

namespace Spark {

/** Pairwise narrow-phase dispatch for <c>IShape2D</c> (Strategy + dispatch table). */
class NarrowPhase2D {
public:
    [[nodiscard]] static bool Overlap(const IShape2D& a, const IShape2D& b);
    [[nodiscard]] static bool ComputeContact(const IShape2D& a, const IShape2D& b, ContactManifold2D& out);
    [[nodiscard]] static bool Raycast(const IShape2D& shape, const Ray2D& ray, float& outDistance);
};

}  // namespace Spark
