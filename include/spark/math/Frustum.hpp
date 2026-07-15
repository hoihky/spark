#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

namespace Spark {

struct AxisAlignedBox;

/**
 * Six clip-space-derived planes in world space for combined column-major view * projection.
 * Inside the volume: plane.DotHomogeneous(point) >= 0 for all planes (point = xyz1).
 */
class Frustum {
public:
    /** Planes from VP where clip = VP * (x,y,z,1)^T and visible satisfies Vulkan-style homogeneous clip. */
    [[nodiscard]] static Frustum FromColumnMajorViewProjection(const Matrix4& viewProjection) noexcept;

    /** Conservative AABB vs frustum; false only if the box is entirely outside one plane. */
    [[nodiscard]] bool IntersectsAxisAlignedBox(const Vector3& boxMin, const Vector3& boxMax) const noexcept;

    [[nodiscard]] bool Intersects(const AxisAlignedBox& box) const noexcept;

private:
    Vector4 planes[6]{};
};

}  // namespace Spark
