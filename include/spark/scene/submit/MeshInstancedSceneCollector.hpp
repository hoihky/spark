#pragma once

#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace Spark {

/** Fills <c>SceneRenderParams::meshInstancedBatches</c> / <c>meshInstances</c> from ECS batch components. */
class MeshInstancedSceneCollector final {
public:
    void CollectInto(
            GameWorld& world,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture) const;
};

}  // namespace Spark