#include "spark/scene/spatial/SpatialBoundsUtil.hpp"

namespace Spark {

namespace SpatialBoundsUtil {

AxisAlignedBox UnionSceneItemsPadded(const AxisAlignedBox* items, const std::size_t count) noexcept {
    if (count == 0 || items == nullptr) {
        return AxisAlignedBox{Vector3::Zero, Vector3::Zero};
    }
    AxisAlignedBox b = items[0];
    for (std::size_t i = 1; i < count; ++i) {
        b = AxisAlignedBox::UnionOf(b, items[i]);
    }
    const Vector3 pad = (b.max - b.min) * 0.02F + Vector3{0.25F, 0.25F, 0.25F};
    return AxisAlignedBox{b.min - pad, b.max + pad};
}

std::uint64_t FingerprintAxisAlignedBoxes(const AxisAlignedBox* items, const std::size_t count) noexcept {
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&h](const std::uint64_t v) noexcept {
        h ^= v;
        h *= 1099511628211ULL;
    };
    mix(static_cast<std::uint64_t>(count));
    if (items == nullptr) {
        return h;
    }
    for (std::size_t i = 0; i < count; ++i) {
        const AxisAlignedBox& box = items[i];
        const std::uint64_t* minWords = reinterpret_cast<const std::uint64_t*>(&box.min.x);
        const std::uint64_t* maxWords = reinterpret_cast<const std::uint64_t*>(&box.max.x);
        mix(minWords[0]);
        mix(minWords[1]);
        mix(maxWords[0]);
        mix(maxWords[1]);
        mix(static_cast<std::uint64_t>(i));
    }
    return h;
}

}  // namespace SpatialBoundsUtil

}  // namespace Spark