#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

namespace Spark {

class GameObject;
struct SceneDrawItem;
struct SceneSpriteDraw;
struct SceneTilemapDraw;
struct SceneWaterDraw;

/** Stable sorts and opaque/transparent partition for scene draw lists (meshes, sprites, tilemaps, water). */
class SceneDrawListSorter final {
public:
    [[nodiscard]] static int OpaqueDrawSortKey(const SceneDrawItem& item) noexcept;

    static void StableSortOpaqueDraws(Array<SceneDrawItem>& items) noexcept;
    static void StableSortSprites(Array<SceneSpriteDraw>& items, SceneSpriteSortMode mode) noexcept;
    static void StableSortTilemaps(Array<SceneTilemapDraw>& draws) noexcept;
    static void StableSortWaterBackToFront(
            Array<SceneWaterDraw>& items,
            const Vector3& cameraPositionWorld,
            SceneWaterSortMode sortMode) noexcept;

    static void PartitionIntoSceneParams(
            const Array<SceneDrawItem>& sortedDrawList,
            SceneRenderParams& params,
            const Vector3& cameraPositionWorld) noexcept;

    [[nodiscard]] static SceneBlendMode ResolveSpriteBlendMode(const GameObject& object) noexcept;
};

}  // namespace Spark