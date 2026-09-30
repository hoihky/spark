#pragma once

#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

namespace Spark {

class GameWorld;
class Texture2D;
struct SceneRenderParams;

/** Builds a tilemap-aligned fog quad for the world sprite pass. */
class FogOfWarScreenPresenter final {
public:
    void AppendWorldSprite(
            SceneRenderParams& params,
            GameWorld& world,
            const SharedPtr<Texture2D>& fogMaskTexture,
            const TilemapGridFrame& frame,
            const SceneSubmitDetail::FindSceneTextureFn& findSceneTexture) const noexcept;
};

}  // namespace Spark
