#pragma once

#include "spark/scene/submit/DrawableSortKey.hpp"
#include "spark/scene/submit/SceneRenderQueue.hpp"

namespace Spark {

class GameObject;
class RenderLayerComponent;
class SortingGroupComponent;
struct SceneDrawItem;
struct SceneSpriteDraw;

/**
 * Resolves effective 2D draw order for sprites and tilemaps from the ECS hierarchy.
 *
 * - <c>RenderLayerComponent</c> selects a named layer and optional order-in-layer.
 * - <c>SortingGroupComponent</c> on an ancestor batches descendants at the group's order
 *   (Unity Sorting Group semantics: nearest enabled group wins).
 * - Drawable-native order (<c>SpriteComponent::sortOrder</c>, <c>TilemapComponent::sortOrderBase</c>)
 *   is used when no <c>RenderLayerComponent</c> overrides order-in-layer.
 */
class DrawableSortResolver {
public:
    [[nodiscard]] static ResolvedDrawableSort Resolve(const GameObject& drawable, std::int32_t nativeOrder) noexcept;

    /** Maps registry sorting layer order to coarse render queue (Background / Opaque / Overlay). */
    [[nodiscard]] static SceneRenderQueue ResolveRenderQueue(std::int16_t sortingLayerOrder) noexcept;

    /** Writes <c>drawableSort</c>, <c>sortWorldYAnchor</c>, and <c>renderQueue</c> on mesh draws. */
    static void ApplyToSceneDrawItem(const GameObject& drawable, SceneDrawItem& item, std::int32_t nativeOrder) noexcept;

    static void ApplyToSpriteDraw(const ResolvedDrawableSort& resolved, SceneSpriteDraw& draw) noexcept;

private:
    [[nodiscard]] static const SortingGroupComponent* FindNearestSortingGroup(const GameObject& drawable) noexcept;
    [[nodiscard]] static std::int16_t ResolveLayerSortingOrder(
            const GameObject& drawable,
            const SortingGroupComponent* sortingGroup) noexcept;
    [[nodiscard]] static std::int32_t ResolveLocalOrderInLayer(
            const GameObject& drawable,
            std::int32_t nativeOrder) noexcept;
    [[nodiscard]] static float ResolveWorldYAnchor(
            const GameObject& drawable,
            const SortingGroupComponent* sortingGroup) noexcept;
};

}  // namespace Spark
