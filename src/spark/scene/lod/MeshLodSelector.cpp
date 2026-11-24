#include "spark/scene/lod/MeshLodSelector.hpp"

#include <algorithm>

namespace Spark {

std::uint32_t MeshLodSelector::SelectIndex(
        const Array<MeshLodLevel>& levels,
        const float cameraDistanceMeters,
        std::uint32_t& activeIndexInOut,
        const float hysteresisMeters) noexcept {
    if (levels.IsEmpty()) {
        return 0U;
    }
    const float distance = std::max(0.0F, cameraDistanceMeters);
  std::uint32_t target = 0U;
    for (std::size_t i = 0; i < levels.GetSize(); ++i) {
        if (distance + 1.0e-4F >= levels[i].minDistance) {
            target = static_cast<std::uint32_t>(i);
        }
    }
    activeIndexInOut = std::min(activeIndexInOut, static_cast<std::uint32_t>(levels.GetSize() - 1U));
    if (target == activeIndexInOut) {
        return activeIndexInOut;
    }
    const float hys = std::max(0.0F, hysteresisMeters);
    if (target > activeIndexInOut) {
        if (distance >= levels[target].minDistance + hys) {
            activeIndexInOut = target;
        }
    } else {
        const float boundary = levels[activeIndexInOut].minDistance;
        if (distance <= boundary - hys) {
            activeIndexInOut = target;
        }
    }
    return activeIndexInOut;
}

}  // namespace Spark