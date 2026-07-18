#include "spark/ecs/components/ai/NavMeshAgentComponent.hpp"

#include "spark/ai/path/GridPathfinder.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/ai/PatrolPathComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"

#include <cmath>

namespace Spark {

namespace {

void FillPatrolPolyline(
        const PatrolPathComponent& path,
        const GameObject& pathOwner,
        Array<Vector2>& outWorldXZ) {
    outWorldXZ.Clear();
    const Matrix4 worldM = pathOwner.GetWorldMatrix();
    for (std::size_t i = 0; i < path.GetWaypoints().GetSize(); ++i) {
        const Vector3 wp = worldM.TransformPoint(path.GetWaypoints()[i]);
        outWorldXZ.PushBack(Vector2{wp.x, wp.z});
    }
    if (path.IsLooping() && outWorldXZ.GetSize() > 1) {
        outWorldXZ.PushBack(outWorldXZ[0]);
    }
}

}  // namespace

void NavMeshAgentComponent::SubsystemTick(GameObject& owner) {
    if (!enabled) {
        return;
    }
    AiAgentComponent* agent = owner.GetComponent<AiAgentComponent>();
    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (agent == nullptr || tr == nullptr) {
        return;
    }

    if (useGridPathfinding) {
        const float goalX = agent->GetBlackboard().GetFloat(0);
        const float goalZ = agent->GetBlackboard().GetFloat(1);
        const Vector3 p = tr->GetLocalTransform().translation;
        const float cell = std::max(gridCellSize, 0.25F);
        GridPathfinder::Cell start{};
        start.x = static_cast<std::int32_t>(std::floor((p.x - gridOriginXZ.x) / cell));
        start.y = static_cast<std::int32_t>(std::floor((p.z - gridOriginXZ.y) / cell));
        GridPathfinder::Cell goal{};
        goal.x = static_cast<std::int32_t>(std::floor((goalX - gridOriginXZ.x) / cell));
        goal.y = static_cast<std::int32_t>(std::floor((goalZ - gridOriginXZ.y) / cell));

        GridBitmapWalkability walk{};
        walk.Resize(std::max(1, gridWidth), std::max(1, gridHeight));
        Array<GridPathfinder::Cell> cells{};
        if (GridPathfinder::FindPath4(walk, start, goal, cells)) {
            agent->ClearPath();
            GridPathfinder::CellsToWorldPolyline(cells, gridOriginXZ, cell, agent->GetPathWorldPolylineXZ());
            agent->SetPathIndex(0);
            return;
        }
    }

    const GameObject* pathHost = patrolPathObject != nullptr ? patrolPathObject : &owner;
    const PatrolPathComponent* path = pathHost->GetComponent<PatrolPathComponent>();
    if (path == nullptr || path->GetWaypoints().IsEmpty()) {
        return;
    }
    FillPatrolPolyline(*path, *pathHost, agent->GetPathWorldPolylineXZ());
    agent->SetPathIndex(0);
}

}  // namespace Spark
