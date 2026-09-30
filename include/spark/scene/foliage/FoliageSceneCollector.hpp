#pragma once

#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

namespace Spark {

class GameWorld;
struct SceneRenderParams;

/** Walks ECS foliage components and fills <c>SceneRenderParams::foliageBatches</c> (F1-05). */
class FoliageSceneCollector {
public:
    void CollectInto(
            GameWorld& world,
            SceneRenderParams& params,
            const SceneSubmitDetail::FindSceneTextureFn& findOrAddTexture) const;
};

}  // namespace Spark
