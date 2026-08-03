#pragma once

#include "spark/physics/core/ContactManifold.hpp"
#include "spark/physics/shapes/IShape2D.hpp"

namespace Spark::ShapeContact2DDetail {

[[nodiscard]] bool OverlapPair(const IShape2D& a, const IShape2D& b) noexcept;
[[nodiscard]] bool ContactPair(const IShape2D& a, const IShape2D& b, ContactManifold2D& out) noexcept;

}  // namespace Spark::ShapeContact2DDetail
