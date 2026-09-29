#include "spark/ai/NavigationSubsystem.hpp"

#include "spark/ecs/components/ai/GridNavAgent2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ai/steering/SteeringBehaviors2D.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

bool ApplyGridNavAgent2DRigidbodyMotion(
        GridNavAgent2DComponent& agent,
        TransformComponent& transform,
        Rigidbody2DComponent& rigidbody,
        const float maxSpeed,
        const float arriveRadius,
        const float deltaTimeSeconds) noexcept {
    if (!agent.HasPath()) {
        return false;
    }
    const Array<Vector2>& waypoints = agent.GetWorldWaypoints();
    if (waypoints.IsEmpty()) {
        return false;
    }

    int index = agent.GetPathIndex();
    if (index < 0) {
        index = 0;
    }
    if (static_cast<std::size_t>(index) >= waypoints.GetSize()) {
        agent.ClearPath();
        return false;
    }

    const Vector3 pos3 = transform.GetLocalTransform().translation;
    const Vector2 pos{pos3.x, pos3.y};
    const Vector2& waypoint = waypoints[static_cast<std::size_t>(index)];
    Vector2 delta{waypoint.x - pos.x, waypoint.y - pos.y};
    const float distSq = delta.x * delta.x + delta.y * delta.y;
    const float arrive = std::max(0.01F, arriveRadius);
    if (distSq <= arrive * arrive) {
        transform.SetTranslation({waypoint.x, waypoint.y, pos3.z});
        ++index;
        agent.SetPathIndex(index);
        if (static_cast<std::size_t>(index) >= waypoints.GetSize()) {
            agent.ClearPath();
            rigidbody.SetVelocity(Vector2::Zero);
            return false;
        }
        return ApplyGridNavAgent2DRigidbodyMotion(
                agent, transform, rigidbody, maxSpeed, arriveRadius, deltaTimeSeconds);
    }

    const float dist = std::sqrt(distSq);
    const float cap = std::max(0.0F, maxSpeed);
    Vector2 velocity = delta.Normalized() * cap;
    if (dist < cap * deltaTimeSeconds * 2.0F) {
        velocity = delta / std::max(deltaTimeSeconds, 1.0e-4F);
        if (velocity.LengthSquared() > cap * cap) {
            velocity = velocity.Normalized() * cap;
        }
    }
    rigidbody.SetVelocity(velocity);
    return true;
}

bool ApplyGridNavAgent2DSteeringMotion(
        GridNavAgent2DComponent& agent,
        TransformComponent& transform,
        Rigidbody2DComponent& rigidbody,
        const float maxSpeed,
        const float arriveRadius,
        const float deltaTimeSeconds) noexcept {
    if (!agent.HasPath()) {
        return false;
    }
    const Array<Vector2>& waypoints = agent.GetWorldWaypoints();
    if (waypoints.IsEmpty()) {
        return false;
    }

    int index = agent.GetPathIndex();
    if (index < 0) {
        index = 0;
    }
    if (static_cast<std::size_t>(index) >= waypoints.GetSize()) {
        agent.ClearPath();
        rigidbody.SetVelocity(Vector2::Zero);
        return false;
    }

    const Vector3 pos3 = transform.GetLocalTransform().translation;
    const Vector2 pos{pos3.x, pos3.y};
    const Vector2& waypoint = waypoints[static_cast<std::size_t>(index)];
    Vector2 delta{waypoint.x - pos.x, waypoint.y - pos.y};
    const float distSq = delta.x * delta.x + delta.y * delta.y;
    const float arrive = std::max(0.01F, arriveRadius);
    if (distSq <= arrive * arrive) {
        transform.SetTranslation({waypoint.x, waypoint.y, pos3.z});
        ++index;
        agent.SetPathIndex(index);
        if (static_cast<std::size_t>(index) >= waypoints.GetSize()) {
            agent.ClearPath();
            rigidbody.SetVelocity(Vector2::Zero);
            return false;
        }
        return ApplyGridNavAgent2DSteeringMotion(
                agent, transform, rigidbody, maxSpeed, arriveRadius, deltaTimeSeconds);
    }

    const Vector2 currentVel = rigidbody.GetVelocity();
    SteeringEnvironment2D env{};
    env.pathPoints = &waypoints;
    env.pathIndex = index;
    env.maxSteeringSpeed = std::max(0.0F, maxSpeed);
    env.maxAcceleration = env.maxSteeringSpeed * 4.0F;
    env.waypointArriveRadius = arrive;
    SteeringPathFollowing2D pathFollow(1.0F);
    Vector2 velocity = pathFollow.ComputeAcceleration(pos, currentVel, env);
    const float cap = std::max(0.0F, maxSpeed);
    if (velocity.LengthSquared() > cap * cap) {
        velocity = velocity.Normalized() * cap;
    }
    rigidbody.SetVelocity(velocity);
    static_cast<void>(deltaTimeSeconds);
    return true;
}

}  // namespace Spark
