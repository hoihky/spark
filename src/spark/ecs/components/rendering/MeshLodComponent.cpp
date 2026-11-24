#include "spark/ecs/components/rendering/MeshLodComponent.hpp"

#include "spark/scene/lod/MeshLodSelector.hpp"

namespace Spark {

std::uint32_t MeshLodComponent::SelectLodIndex(const float cameraDistanceMeters) noexcept {
    if (!enabled || levels.IsEmpty()) {
        return 0U;
    }
    return MeshLodSelector::SelectIndex(levels, cameraDistanceMeters, activeLodIndex, hysteresisMeters);
}

void MeshLodComponent::Clear() noexcept {
    levels.Clear();
    activeLodIndex = 0U;
}

}  // namespace Spark