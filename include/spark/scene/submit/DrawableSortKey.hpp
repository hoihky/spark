#pragma once

#include <cstdint>

namespace Spark {

/** Composite 2D sort key resolved from ECS render layers and sorting groups. */
struct DrawableSortKey {
    /** Global layer order from <c>RenderLayerRegistry</c> (lower draws behind). */
    std::int16_t sortingLayerOrder = 0;
    /** Order within the sorting layer (lower draws behind). */
    std::int32_t sortingOrder = 0;
};

struct ResolvedDrawableSort {
    DrawableSortKey key{};
    /** World-space Y used for <c>SceneSpriteSortMode::SortOrderThenWorldY</c> tie-breaks. */
    float worldYAnchor = 0.0F;
};

/** Returns true when <c>a</c> should rasterize in front of (after) <c>b</c>. */
[[nodiscard]] inline bool DrawableSortMoreInFront(const DrawableSortKey& a, const DrawableSortKey& b) noexcept {
    if (a.sortingLayerOrder != b.sortingLayerOrder) {
        return a.sortingLayerOrder > b.sortingLayerOrder;
    }
    return a.sortingOrder > b.sortingOrder;
}

}  // namespace Spark
