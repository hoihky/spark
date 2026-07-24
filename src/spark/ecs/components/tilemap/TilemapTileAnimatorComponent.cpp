#include "spark/ecs/components/tilemap/TilemapTileAnimatorComponent.hpp"

namespace Spark {

void TilemapTileAnimatorComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& /*owner*/,
        IEngineContext& /*context*/) {
    if (!playing) {
        return;
    }
    animationTimeSeconds += timing.deltaTimeSeconds;
}

}  // namespace Spark
