#pragma once

#include "spark/ecs/GameComponent.hpp"

namespace Spark {

/**
 * Batches descendant 2D drawables into a single sorting unit (Unity Sorting Group).
 * The nearest enabled group on the parent chain wins.
 *
 * Descendants compare using the group's <c>sortingOrder</c> plus their local order-in-layer,
 * on the sorting layer resolved from the group owner (or the drawable's own layer).
 */
class SortingGroupComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SortingGroup;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SortingGroupComponent(std::int32_t sortingOrderIn = 0) noexcept : sortingOrder(sortingOrderIn) {}

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    [[nodiscard]] std::int32_t GetSortingOrder() const noexcept { return sortingOrder; }
    [[nodiscard]] bool UsesRootWorldY() const noexcept { return sortAtRootWorldY; }

    void SetEnabled(const bool value) noexcept { enabled = value; }
    void SetSortingOrder(const std::int32_t order) noexcept { sortingOrder = order; }
    void SetSortAtRootWorldY(const bool value) noexcept { sortAtRootWorldY = value; }

private:
    bool enabled = true;
    bool sortAtRootWorldY = true;
    std::int32_t sortingOrder = 0;
};

}  // namespace Spark
