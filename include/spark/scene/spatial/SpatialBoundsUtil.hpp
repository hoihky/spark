#pragma once

#include "spark/math/AxisAlignedBox.hpp"
#include "spark/math/Vector3.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

namespace SpatialBoundsUtil {

/** Padded union of item bounds (2% extent + 0.25 m) for octree/BVH roots. */
[[nodiscard]] AxisAlignedBox UnionSceneItemsPadded(const AxisAlignedBox* items, std::size_t count) noexcept;

[[nodiscard]] std::uint64_t FingerprintAxisAlignedBoxes(const AxisAlignedBox* items, std::size_t count) noexcept;

}  // namespace SpatialBoundsUtil

}  // namespace Spark