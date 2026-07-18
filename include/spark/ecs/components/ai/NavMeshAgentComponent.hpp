#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

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

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    void SetEnabled(const bool e) noexcept { enabled = e; }

    [[nodiscard]] GameObject* GetPatrolPathObject() const noexcept { return patrolPathObject; }
    void SetPatrolPathObject(GameObject* o) noexcept { patrolPathObject = o; }

    [[nodiscard]] bool UseGridPathfinding() const noexcept { return useGridPathfinding; }
    void SetUseGridPathfinding(const bool u) noexcept { useGridPathfinding = u; }

    [[nodiscard]] Vector2 GetGridOriginXZ() const noexcept { return gridOriginXZ; }
    void SetGridOriginXZ(const Vector2& o) noexcept { gridOriginXZ = o; }

    [[nodiscard]] float GetGridCellSize() const noexcept { return gridCellSize; }
    void SetGridCellSize(const float s) noexcept { gridCellSize = s; }

    [[nodiscard]] std::int32_t GetGridWidth() const noexcept { return gridWidth; }
    void SetGridWidth(const std::int32_t w) noexcept { gridWidth = w; }

    [[nodiscard]] std::int32_t GetGridHeight() const noexcept { return gridHeight; }
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
