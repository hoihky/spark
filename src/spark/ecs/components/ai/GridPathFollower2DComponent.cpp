#include "spark/ecs/components/ai/GridPathFollower2DComponent.hpp"

#include "spark/ecs/components/ai/GridNavAgent2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

void GridPathFollower2DComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& context) {
    (void)context;
    if (!enabled) {
        return;
    }

    GridNavAgent2DComponent* agent = navAgent;
    if (agent == nullptr) {
        agent = owner.GetComponent<GridNavAgent2DComponent>();
    }
    if (agent == nullptr || !agent->HasPath()) {
        return;
    }

    const Array<Vector2>& waypoints = agent->GetWorldWaypoints();
    if (waypoints.IsEmpty()) {
        return;
    }

    int index = agent->GetPathIndex();
    if (index < 0) {
        index = 0;
    }
    if (static_cast<std::size_t>(index) >= waypoints.GetSize()) {
        if (stopOnPathEnd) {
            agent->ClearPath();
        }
        return;
    }

    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }

    const Vector3 pos3 = tr->GetLocalTransform().translation;
    const Vector2 pos{pos3.x, pos3.y};
    const Vector2& waypoint = waypoints[static_cast<std::size_t>(index)];
    Vector2 delta{waypoint.x - pos.x, waypoint.y - pos.y};
    const float distSq = delta.x * delta.x + delta.y * delta.y;
    const float arrive = std::max(0.01F, arriveRadius);
    if (distSq <= arrive * arrive) {
        tr->SetTranslation({waypoint.x, waypoint.y, pos3.z});
        ++index;
        agent->SetPathIndex(index);
        if (static_cast<std::size_t>(index) >= waypoints.GetSize()) {
            if (stopOnPathEnd) {
                agent->ClearPath();
            }
        }
        return;
    }

    const float dist = std::sqrt(distSq);
    const float step = std::max(0.0F, maxSpeed) * timing.deltaTimeSeconds;
    const float moveScale = std::min(1.0F, step / std::max(dist, 1.0e-4F));
    const Vector2 move{delta.x * moveScale, delta.y * moveScale};

    if (mode == GridPathFollower2DMode::Rigidbody2DVelocity) {
        if (Rigidbody2DComponent* rb = owner.GetComponent<Rigidbody2DComponent>()) {
            const float dt = std::max(timing.deltaTimeSeconds, 1.0e-4F);
            Vector2 v{move.x / dt, move.y / dt};
            const float cap = std::max(0.0F, maxSpeed);
            if (v.LengthSquared() > cap * cap) {
                v = v.Normalized() * cap;
            }
            rb->SetVelocity(v);
            return;
        }
    }

    tr->SetTranslation({pos.x + move.x, pos.y + move.y, pos3.z});
}

}  // namespace Spark
