#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Marks a <c>GameObject</c> as a grid-navigation goal for <c>GridNavAgent2DComponent</c>.
 * Agents resolve the goal cell from this object's world XY position each repath.
 */
class GridNavTarget2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GridNavTarget2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool value) noexcept { enabled = value; }
    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    /** When true, agents snap the goal to the walkable cell nearest this transform. */
    SPARK_SCRIPT_BIND(set_snap_to_walkable_cell)
    void SetSnapToWalkableCell(const bool value) noexcept { snapToWalkableCell = value; }
    SPARK_SCRIPT_BIND(get_snap_to_walkable_cell)
    [[nodiscard]] bool GetSnapToWalkableCell() const noexcept { return snapToWalkableCell; }

private:
    bool enabled = true;
    bool snapToWalkableCell = true;
};

}  // namespace Spark
