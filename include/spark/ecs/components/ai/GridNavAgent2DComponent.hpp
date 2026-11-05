#pragma once

#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/core/Array.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>

namespace Spark {

class GameObject;

/** How <c>GridNavAgent2DComponent</c> chooses the goal cell each repath. */
enum class GridNavGoalMode2D : std::uint8_t {
    /** Follow <c>goalTarget</c> transform XY (requires <c>GridNavTarget2DComponent</c> when filtering). */
    TargetObject = 0,
    /** Fixed world position on the tilemap XY plane. */
    WorldPosition = 1,
    /** Explicit grid cell on the linked <c>TilemapGameplayGridComponent</c>. */
    GridCell = 2,
};

/**
 * Tilemap-grid A* for 2D top-down / platformer maps (XY plane via <c>TilemapGridFrame</c>).
 * Run <c>ProcessGridNavAgents2D</c> each frame before movement (e.g. at the start of <c>SimulateGameAi</c>).
 *
 * Optionally mirrors the polyline into a sibling <c>AiAgentComponent</c> for steering-based motion.
 */
class GridNavAgent2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GridNavAgent2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void SetEnabled(const bool value) noexcept { enabled = value; }
    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }

    void SetGridSourceObject(GameObject* object) noexcept { gridSourceObject = object; }
    [[nodiscard]] GameObject* GetGridSourceObject() const noexcept { return gridSourceObject; }

    void SetGoalMode(const GridNavGoalMode2D mode) noexcept { goalMode = mode; }
    [[nodiscard]] GridNavGoalMode2D GetGoalMode() const noexcept { return goalMode; }

    void SetGoalTarget(GameObject* object) noexcept { goalTarget = object; }
    [[nodiscard]] GameObject* GetGoalTarget() const noexcept { return goalTarget; }

    void SetGoalWorldPosition(const Vector2& position) noexcept { goalWorldPosition = position; }
    [[nodiscard]] const Vector2& GetGoalWorldPosition() const noexcept { return goalWorldPosition; }

    void SetGoalCell(const GridPathfinder::Cell& cell) noexcept { goalCell = cell; }
    [[nodiscard]] const GridPathfinder::Cell& GetGoalCell() const noexcept { return goalCell; }

    void SetRepathEveryFrame(const bool value) noexcept { repathEveryFrame = value; }
    [[nodiscard]] bool GetRepathEveryFrame() const noexcept { return repathEveryFrame; }

    void SetRepathIntervalSeconds(const float seconds) noexcept { repathIntervalSeconds = seconds; }
    [[nodiscard]] float GetRepathIntervalSeconds() const noexcept { return repathIntervalSeconds; }

    /** When true, copies the path into <c>AiAgentComponent::GetPathWorldPolylineXZ()</c> if present. */
    void SetSyncToAiAgent(const bool value) noexcept { syncToAiAgent = value; }
    [[nodiscard]] bool GetSyncToAiAgent() const noexcept { return syncToAiAgent; }

    void RequestRepath() noexcept { repathRequested = true; }
    void ClearPath() noexcept;

    [[nodiscard]] bool HasPath() const noexcept { return hasPath; }
    [[nodiscard]] int GetPathIndex() const noexcept { return pathIndex; }
    void SetPathIndex(const int index) noexcept { pathIndex = index; }

    [[nodiscard]] const Array<GridPathfinder::Cell>& GetPathCells() const noexcept { return pathCells; }
    [[nodiscard]] const Array<Vector2>& GetWorldWaypoints() const noexcept { return worldWaypoints; }

    void SubsystemTick(GameObject& owner, float deltaTimeSeconds) noexcept;

private:
    [[nodiscard]] bool ShouldRepath(
            const GridPathfinder::Cell& goalCell,
            float deltaTimeSeconds) noexcept;

    GameObject* gridSourceObject = nullptr;
    GameObject* goalTarget = nullptr;
    GridNavGoalMode2D goalMode = GridNavGoalMode2D::TargetObject;
    Vector2 goalWorldPosition{0.0F, 0.0F};
    GridPathfinder::Cell goalCell{};
    GridPathfinder::Cell lastGoalCell{};
    Array<GridPathfinder::Cell> pathCells{};
    Array<Vector2> worldWaypoints{};
    bool enabled = true;
    bool hasPath = false;
    bool repathEveryFrame = true;
    bool repathRequested = true;
    bool syncToAiAgent = true;
    int pathIndex = 0;
    float repathIntervalSeconds = 0.25F;
    float repathTimer = 0.0F;
};

}  // namespace Spark
