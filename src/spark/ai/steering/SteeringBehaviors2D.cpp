#include "spark/ai/steering/SteeringBehaviors2D.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector2 ClampLen(const Vector2 v, const float maxLen) noexcept {
    const float lenSq = v.LengthSquared();
    if (lenSq <= maxLen * maxLen || lenSq < 1.0e-8F) {
        return v;
    }
    return v.Normalized() * maxLen;
}

[[nodiscard]] Vector2 SteerVelocity(
        const Vector2& desiredVel,
        const Vector2& currentVel,
        const SteeringEnvironment2D& env,
        const float scale) noexcept {
    const Vector2 delta = desiredVel - currentVel;
    return ClampLen(delta, env.maxAcceleration) * scale;
}

}  // namespace

Vector2 SteeringPathFollowing2D::ComputeAcceleration(
        const Vector2& positionXY,
        const Vector2& velocityXY,
        const SteeringEnvironment2D& env) const noexcept {
    if (env.pathPoints == nullptr || env.pathPoints->IsEmpty()) {
        return Vector2::Zero;
    }
    const int n = static_cast<int>(env.pathPoints->GetSize());
    int idx = env.pathIndex;
    if (idx < 0) {
        idx = 0;
    }
    if (idx >= n) {
        idx = n - 1;
    }
    const Vector2 wp = (*env.pathPoints)[static_cast<std::size_t>(idx)];
    Vector2 offset = wp - positionXY;
    const float dist = offset.Length();
    if (dist < 1.0e-5F) {
        return Vector2::Zero;
    }
    const float arrive = std::max(0.05F, env.waypointArriveRadius);
    float speed = env.maxSteeringSpeed;
    if (dist < arrive) {
        speed *= dist / arrive;
    }
    const Vector2 desired = offset.Normalized() * speed;
    return SteerVelocity(desired, velocityXY, env, weight);
}

Vector2 SteeringObstacleAvoidance2D::ComputeAcceleration(
        const Vector2& positionXY,
        const Vector2& velocityXY,
        const SteeringEnvironment2D& env) const noexcept {
    if (env.obstacleCenters == nullptr || env.obstacleRadii == nullptr) {
        return Vector2::Zero;
    }
    Vector2 fwd = velocityXY;
    if (fwd.LengthSquared() < 1.0e-4F) {
        fwd = Vector2{1.0F, 0.0F};
    } else {
        fwd = fwd.Normalized();
    }
    const float look = env.obstacleAvoidLookahead > 0.5F ? env.obstacleAvoidLookahead : 0.5F;
    Vector2 steer{0.0F, 0.0F};
    for (std::size_t i = 0; i < env.obstacleCenters->GetSize(); ++i) {
        const Vector2& c = (*env.obstacleCenters)[i];
        const float r = i < env.obstacleRadii->GetSize() ? (*env.obstacleRadii)[i] : 1.0F;
        const Vector2 local = c - positionXY;
        const float proj = Vector2::Dot(local, fwd);
        if (proj < 0.0F || proj > look + r) {
            continue;
        }
        const Vector2 closest = fwd * proj;
        const Vector2 perp = local - closest;
        const float d = perp.Length();
        const float threat = r + 0.25F;
        if (d > threat + 0.1F) {
            continue;
        }
        Vector2 side = perp;
        if (side.LengthSquared() < 1.0e-8F) {
            side = Vector2{-fwd.y, fwd.x};
        } else {
            side = side.Normalized();
        }
        const float mag = (threat - d) / threat;
        steer += side * (mag * env.obstacleAvoidSideWeight);
    }
    return ClampLen(steer, env.maxAcceleration) * weight;
}

}  // namespace Spark
