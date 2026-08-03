#pragma once

#include "spark/physics/core/ContactManifold.hpp"
#include "spark/physics/shapes/IShape3D.hpp"

namespace Spark::ShapeContact3DDetail {

[[nodiscard]] bool OverlapPair(const IShape3D& a, const IShape3D& b) noexcept;
[[nodiscard]] bool ContactPair(const IShape3D& a, const IShape3D& b, ContactManifold3D& out) noexcept;

}  // namespace Spark::ShapeContact3DDetail
