#include "spark/ecs/components/ai/GridNavAgent2DComponent.hpp"

#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/ai/GridNavTarget2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/tilemap/TilemapGridCoordinates.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] GridPathfinder::Cell ClampCellToGrid(
        const GridPathfinder::Cell& cell,
        const TilemapGridFrame& frame) noexcept {
    GridPathfinder::Cell out = cell;
    if (frame.mapWidth > 0) {
        const std::int32_t maxX = static_cast<std::int32_t>(frame.mapWidth) - 1;
        out.x = std::clamp(out.x, 0, maxX);
    }
    if (frame.mapHeight > 0) {
        const std::int32_t maxY = static_cast<std::int32_t>(frame.mapHeight) - 1;
        out.y = std::clamp(out.y, 0, maxY);
    }
    return out;
}

[[nodiscard]] GridPathfinder::Cell FindNearestWalkableCell(
        const IGridWalkability& walk,
        const TilemapGridFrame& frame,
        GridPathfinder::Cell seed) noexcept {
    seed = ClampCellToGrid(seed, frame);
    if (walk.IsWalkable(seed.x, seed.y)) {
        return seed;
    }
    for (int radius = 1; radius <= 8; ++radius) {
        for (int dy = -radius; dy <= radius; ++dy) {
            for (int dx = -radius; dx <= radius; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                    continue;
                }
                GridPathfinder::Cell candidate{seed.x + dx, seed.y + dy};
                if (!frame.IsCellInBounds(candidate)) {
                    continue;
                }
                if (walk.IsWalkable(candidate.x, candidate.y)) {
                    return candidate;
                }
            }
        }
    }
    return seed;
}

void BuildWorldWaypoints(
        const Array<GridPathfinder::Cell>& cells,
        const TilemapGridFrame& frame,
        Array<Vector2>& outWaypoints) {
    outWaypoints.Clear();
    for (std::size_t i = 0; i < cells.GetSize(); ++i) {
        outWaypoints.PushBack(frame.CellCenterToWorldXY(cells[i]));
    }
}

void SyncPathToAiAgent(GameObject& owner, const Array<Vector2>& waypoints, const int pathIndex) noexcept {
    AiAgentComponent* agent = owner.GetComponent<AiAgentComponent>();
    if (agent == nullptr) {
        return;
    }
    agent->ClearPath();
    Array<Vector2>& poly = agent->GetPathWorldPolylineXZ();
    for (std::size_t i = 0; i < waypoints.GetSize(); ++i) {
        poly.PushBack(waypoints[i]);
    }
    agent->SetPathIndex(pathIndex);
}

}  // namespace

void GridNavAgent2DComponent::ClearPath() noexcept {
    pathCells.Clear();
    worldWaypoints.Clear();
    hasPath = false;
    pathIndex = 0;
}

bool GridNavAgent2DComponent::ShouldRepath(
        const GridPathfinder::Cell& goal,
        const float deltaTimeSeconds) noexcept {
    if (repathRequested) {
        repathRequested = false;
        return true;
    }
    if (repathEveryFrame) {
        return true;
    }
    if (goal.x != lastGoalCell.x || goal.y != lastGoalCell.y) {
        return true;
    }
    repathTimer += deltaTimeSeconds;
    if (repathTimer >= repathIntervalSeconds) {
        repathTimer = 0.0F;
        return true;
    }
    return false;
}

void GridNavAgent2DComponent::SubsystemTick(GameObject& owner, const float deltaTimeSeconds) noexcept {
    if (!enabled) {
        return;
    }
    if (gridSourceObject == nullptr) {
        return;
    }
    TilemapGameplayGridComponent* gridComp = gridSourceObject->GetComponent<TilemapGameplayGridComponent>();
    if (gridComp == nullptr) {
        return;
    }
    gridComp->RebakeIfNeeded(*gridSourceObject);

    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }

    const TilemapGridFrame& frame = gridComp->GetGridFrame();
    const IGridWalkability& walk = gridComp->GetWalkability();

    GridPathfinder::Cell resolvedGoal{};
    bool snapGoal = true;
    switch (goalMode) {
        case GridNavGoalMode2D::TargetObject: {
            if (goalTarget == nullptr) {
                return;
            }
            const TransformComponent* goalTr = goalTarget->GetComponent<TransformComponent>();
            if (goalTr == nullptr) {
                return;
            }
            if (const GridNavTarget2DComponent* target = goalTarget->GetComponent<GridNavTarget2DComponent>()) {
                if (!target->IsEnabled()) {
                    return;
                }
                snapGoal = target->GetSnapToWalkableCell();
            }
            resolvedGoal = frame.WorldPositionToCell(goalTr->GetLocalTransform().translation);
            break;
        }
        case GridNavGoalMode2D::WorldPosition:
            resolvedGoal = frame.WorldXYToCell(goalWorldPosition);
            break;
        case GridNavGoalMode2D::GridCell:
            resolvedGoal = goalCell;
            break;
    }

    if (snapGoal) {
        resolvedGoal = FindNearestWalkableCell(walk, frame, resolvedGoal);
    }

    if (!ShouldRepath(resolvedGoal, deltaTimeSeconds)) {
        return;
    }

    lastGoalCell = resolvedGoal;
    const GridPathfinder::Cell start = frame.WorldPositionToCell(tr->GetLocalTransform().translation);
    GridPathfinder::Cell walkableStart = FindNearestWalkableCell(walk, frame, start);

    Array<GridPathfinder::Cell> cells{};
    if (!GridPathfinder::FindPath4(walk, walkableStart, resolvedGoal, cells)) {
        ClearPath();
        return;
    }

    pathCells = cells;
    BuildWorldWaypoints(pathCells, frame, worldWaypoints);
    hasPath = !worldWaypoints.IsEmpty();
    pathIndex = worldWaypoints.GetSize() > 1 ? 1 : 0;

    if (syncToAiAgent) {
        SyncPathToAiAgent(owner, worldWaypoints, pathIndex);
    }
}

}  // namespace Spark
