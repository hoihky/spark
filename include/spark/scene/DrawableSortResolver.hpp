#pragma once

#include "spark/scene/DrawableSortKey.hpp"

namespace Spark {

class GameObject;
class RenderLayerComponent;
class SortingGroupComponent;

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
