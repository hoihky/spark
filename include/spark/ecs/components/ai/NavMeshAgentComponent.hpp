#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/**
 * Feeds <c>AiAgentComponent::pathWorldXZ</c> from a linked <c>PatrolPathComponent</c> or optional grid pathfinding.
 * Run via <c>ProcessNavMeshAgents</c> before <c>SimulateGameAi</c>.
 */
class NavMeshAgentComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::NavMeshAgent;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(is_enabled)
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(set_enabled)
    void SetEnabled(const bool e) noexcept { enabled = e; }

    SPARK_SCRIPT_BIND(get_patrol_path_object)
    [[nodiscard]] GameObject* GetPatrolPathObject() const noexcept { return patrolPathObject; }
    SPARK_SCRIPT_BIND(set_patrol_path_object)
    void SetPatrolPathObject(GameObject* o) noexcept { patrolPathObject = o; }

    SPARK_SCRIPT_BIND(is_use_grid_pathfinding)
    [[nodiscard]] bool UseGridPathfinding() const noexcept { return useGridPathfinding; }
    SPARK_SCRIPT_BIND(set_use_grid_pathfinding)
    void SetUseGridPathfinding(const bool u) noexcept { useGridPathfinding = u; }

    SPARK_SCRIPT_BIND(get_grid_origin_x_z)
    [[nodiscard]] Vector2 GetGridOriginXZ() const noexcept { return gridOriginXZ; }
    SPARK_SCRIPT_BIND(set_grid_origin_x_z)
    void SetGridOriginXZ(const Vector2& o) noexcept { gridOriginXZ = o; }

    SPARK_SCRIPT_BIND(get_grid_cell_size)
    [[nodiscard]] float GetGridCellSize() const noexcept { return gridCellSize; }
    SPARK_SCRIPT_BIND(set_grid_cell_size)
    void SetGridCellSize(const float s) noexcept { gridCellSize = s; }

    SPARK_SCRIPT_BIND(get_grid_width)
    [[nodiscard]] std::int32_t GetGridWidth() const noexcept { return gridWidth; }
    SPARK_SCRIPT_BIND(set_grid_width)
    void SetGridWidth(const std::int32_t w) noexcept { gridWidth = w; }

    SPARK_SCRIPT_BIND(get_grid_height)
    [[nodiscard]] std::int32_t GetGridHeight() const noexcept { return gridHeight; }
    SPARK_SCRIPT_BIND(set_grid_height)
    void SetGridHeight(const std::int32_t h) noexcept { gridHeight = h; }

    void SubsystemTick(GameObject& owner);

private:
    GameObject* patrolPathObject = nullptr;
    bool enabled = true;
    bool useGridPathfinding = false;
    Vector2 gridOriginXZ{0.0F, 0.0F};
    float gridCellSize = 1.0F;
    std::int32_t gridWidth = 64;
    std::int32_t gridHeight = 64;
};

}  // namespace Spark
