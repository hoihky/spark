#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"
#include "spark/scene/SceneSpriteTileCull.hpp"
#include "spark/scene/Texture2D.hpp"

#include <functional>

namespace Spark {

class GameWorld;
class TilemapComponent;

/**
 * Walks ECS tilemaps and fills <c>SceneRenderParams::tilemaps</c> plus the shared
 * <c>tilemapTiles</c> pool with view-culled tile instances.
 */
class SceneTilemapSubmitter {
public:
    using FindTextureLayerFn = std::function<std::int32_t(const SharedPtr<Texture2D>&)>;
    using ResolveBlendModeFn = std::function<SceneBlendMode(const GameObject&)>;

    void Submit(
            GameWorld& world,
            SceneRenderParams& params,
            const SceneSpriteTileCull& cull,
            const FindTextureLayerFn& findTextureLayer,
            const ResolveBlendModeFn& resolveBlendMode) const noexcept;

    static void StableSortTilemapDraws(Array<SceneTilemapDraw>& draws) noexcept;
};

}  // namespace Spark
