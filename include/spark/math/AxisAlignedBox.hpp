#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

/** World- or model-space axis-aligned box (min inclusive, max inclusive). */
struct AxisAlignedBox {
    Vector3 min{};
    Vector3 max{};

    [[nodiscard]] static AxisAlignedBox FromMinMax(const Vector3& a, const Vector3& b) noexcept {
        return {Vector3{std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)},
                Vector3{std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)}};
    }

    [[nodiscard]] static AxisAlignedBox UnionOf(const AxisAlignedBox& a, const AxisAlignedBox& b) noexcept {
        return FromMinMax(
                Vector3{std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y), std::min(a.min.z, b.min.z)},
                Vector3{std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y), std::max(a.max.z, b.max.z)});
    }

    /** Tight world AABB around an oriented local AABB after applying `world` (column-major). */
    static void EncapsulateTransformedLocal(
            const Vector3& localMin, const Vector3& localMax, const Matrix4& world, Vector3& outMin, Vector3& outMax)
            noexcept;
};

}  // namespace Spark
