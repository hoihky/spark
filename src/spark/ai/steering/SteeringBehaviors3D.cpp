#include "spark/ai/steering/SteeringBehaviors3D.hpp"

#include "spark/ai/AiBlackboard.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] Vector3 ClampLen(const Vector3& v, const float maxLen) noexcept {
    const float m2 = maxLen * maxLen;
    if (v.LengthSquared() <= m2) {
        return v;
    }
    return v.Normalized() * maxLen;
}

[[nodiscard]] Vector3 SeekDir(const Vector3& from, const Vector3& to) noexcept {
    const Vector3 d = to - from;
    if (d.LengthSquared() < 1.0e-10F) {
        return Vector3::Zero;
    }
    return d.Normalized();
}

/** Linear steering acceleration: pull velocity toward desiredVelocity, capped by maxAcceleration. */
[[nodiscard]] Vector3 SteerVelocity(
        const Vector3& desiredVelocity,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        const float weight) noexcept {
    const Vector3 a = (desiredVelocity - velocity) * weight;
    return ClampLen(a, env.maxAcceleration);
}

void LeaderBasisXZ(const Vector3& leaderVel, Vector3& outRight, Vector3& outForward) noexcept {
    Vector3 fwd = leaderVel;
    if (fwd.LengthSquared() < 1.0e-6F) {
        fwd = Vector3{0.0F, 0.0F, -1.0F};
    } else {
        fwd.y = 0.0F;
        if (fwd.LengthSquared() < 1.0e-6F) {
            fwd = Vector3{0.0F, 0.0F, -1.0F};
        } else {
            fwd = fwd.Normalized();
        }
    }
    outForward = fwd;
    outRight = Vector3::Cross(Vector3::UnitY, fwd).Normalized();
}

}  // namespace

Vector3 SteeringSeek3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    const Vector3 dir = SeekDir(position, env.targetPosition);
    if (dir.LengthSquared() < 1.0e-12F) {
        return Vector3::Zero;
    }
    const Vector3 desired = dir * env.maxSteeringSpeed;
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringFlee3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    const Vector3 away = position - env.targetPosition;
    if (away.LengthSquared() < 1.0e-10F) {
        return Vector3::Zero;
    }
    const Vector3 desired = away.Normalized() * env.maxSteeringSpeed;
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringArrive3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    const Vector3 offset = env.targetPosition - position;
    const float dist = offset.Length();
    if (dist < 1.0e-5F) {
        return ClampLen(-velocity * weight, env.maxAcceleration);
    }
    const float slowR = env.arriveSlowRadius > 0.05F ? env.arriveSlowRadius : 0.05F;
    float speed = 1.0F;
    if (dist < slowR) {
        speed = dist / slowR;
    }
    const Vector3 desired = offset.Normalized() * (speed * env.maxSteeringSpeed);
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringPursuit3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    const Vector3 toT = env.targetPosition - position;
    const float dist = toT.Length();
    const float rel = env.targetVelocity.Length();
    float t = rel > 0.1F ? dist / rel : 0.0F;
    if (t > 2.5F) {
        t = 2.5F;
    }
    const Vector3 predicted = env.targetPosition + env.targetVelocity * t;
    const Vector3 dir = SeekDir(position, predicted);
    if (dir.LengthSquared() < 1.0e-12F) {
        return Vector3::Zero;
    }
    const Vector3 desired = dir * env.maxSteeringSpeed;
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringEvade3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    const Vector3 toP = env.pursuerPosition - position;
    const float dist = toP.Length();
    const float rel = env.pursuerVelocity.Length();
    float t = rel > 0.1F ? dist / rel : 0.0F;
    if (t > 2.0F) {
        t = 2.0F;
    }
    const Vector3 predicted = env.pursuerPosition + env.pursuerVelocity * t;
    const Vector3 away = position - predicted;
    if (away.LengthSquared() < 1.0e-10F) {
        return Vector3::Zero;
    }
    const Vector3 desired = away.Normalized() * env.maxSteeringSpeed;
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringWander3D::Compute(
        const Vector3& /*position*/,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& board) const {
    int seed = board.GetInt(0);
    seed = seed * 1103515245 + 12345;
    board.SetInt(0, seed);
    const float u1 = static_cast<float>((seed >> 8) & 0xffff) / 65535.0F;
    const float u2 = static_cast<float>((seed >> 20) & 0xffff) / 65535.0F;
    const float u3 = static_cast<float>((seed >> 3) & 0xffff) / 65535.0F;
    const float twoPi = 6.2831855F;
    const float theta = u1 * twoPi;
    const float phi = u2 * twoPi;
    Vector3 dir{std::cos(theta) * std::cos(phi), std::sin(phi) * 0.35F, std::sin(theta) * std::cos(phi)};
    if (velocity.LengthSquared() > 1.0e-6F) {
        const Vector3 base = velocity.Normalized();
        const Vector3 side = Vector3::Cross(base, Vector3::UnitY).Normalized();
        const Vector3 up = Vector3::Cross(side, base).Normalized();
        dir = side * dir.x + up * dir.y + base * std::fabs(dir.z);
    } else {
        dir = dir.Normalized();
    }
    const float wanderSpeed = env.maxSteeringSpeed * (0.35F + 0.25F * u3);
    const Vector3 desired = dir * wanderSpeed;
    return SteerVelocity(desired, velocity, env, 0.85F * weight);
}

Vector3 SteeringObstacleAvoidance3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    if (env.obstacleCenters == nullptr || env.obstacleRadii == nullptr) {
        return Vector3::Zero;
    }
    Vector3 fwd = velocity;
    if (fwd.LengthSquared() < 1.0e-4F) {
        fwd = Vector3{0.0F, 0.0F, -1.0F};
    } else {
        fwd = fwd.Normalized();
    }
    const float look = env.obstacleAvoidLookahead > 0.5F ? env.obstacleAvoidLookahead : 0.5F;
    Vector3 steer{0.0F, 0.0F, 0.0F};
    for (std::size_t i = 0; i < env.obstacleCenters->GetSize(); ++i) {
        const Vector3& c = (*env.obstacleCenters)[i];
        const float r = i < env.obstacleRadii->GetSize() ? (*env.obstacleRadii)[i] : 1.0F;
        const Vector3 local = c - position;
        const float proj = Vector3::Dot(local, fwd);
        if (proj < 0.0F || proj > look + r) {
            continue;
        }
        const Vector3 closest = fwd * proj;
        const Vector3 perp = local - closest;
        const float d = perp.Length();
        const float threat = r + 0.25F;
        if (d > threat + 0.1F) {
            continue;
        }
        Vector3 side = perp;
        if (side.LengthSquared() < 1.0e-8F) {
            side = Vector3::Cross(fwd, Vector3::UnitY).Normalized();
        } else {
            side = side.Normalized();
        }
        const float mag = (threat - d) / threat;
        steer += side * (mag * env.obstacleAvoidSideWeight);
    }
    return ClampLen(steer, env.maxAcceleration) * weight;
}

Vector3 SteeringWallAvoidance3D::Compute(
        const Vector3& position,
        const Vector3& /*velocity*/,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    const float m = env.wallAvoidMargin > 0.1F ? env.wallAvoidMargin : 0.1F;
    Vector3 acc{0.0F, 0.0F, 0.0F};
    auto push = [&](const float distToMin, const float distToMax, const int axis, const float signMin,
                    const float signMax) noexcept {
        if (distToMin < m) {
            const float u = (m - distToMin) / m;
            acc[axis] += signMin * u;
        }
        if (distToMax < m) {
            const float u = (m - distToMax) / m;
            acc[axis] += signMax * u;
        }
    };
    push(position.x - env.worldBoundsMin.x, env.worldBoundsMax.x - position.x, 0, 1.0F, -1.0F);
    push(position.y - env.worldBoundsMin.y, env.worldBoundsMax.y - position.y, 1, 1.0F, -1.0F);
    push(position.z - env.worldBoundsMin.z, env.worldBoundsMax.z - position.z, 2, 1.0F, -1.0F);
    return ClampLen(acc, env.maxAcceleration) * weight;
}

Vector3 SteeringInterpose3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    const Vector3 mid = (env.secondaryPosition + env.targetPosition) * 0.5F;
    const Vector3 dir = SeekDir(position, mid);
    if (dir.LengthSquared() < 1.0e-12F) {
        return Vector3::Zero;
    }
    const Vector3 desired = dir * env.maxSteeringSpeed;
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringHide3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    if (env.obstacleCenters == nullptr || env.obstacleRadii == nullptr) {
        return Vector3::Zero;
    }
    Vector3 bestSpot = env.targetPosition;
    float bestD2 = 1.0e30F;
    for (std::size_t i = 0; i < env.obstacleCenters->GetSize(); ++i) {
        const Vector3& c = (*env.obstacleCenters)[i];
        const float r = i < env.obstacleRadii->GetSize() ? (*env.obstacleRadii)[i] : 1.0F;
        Vector3 fromP = c - env.pursuerPosition;
        if (fromP.LengthSquared() < 1.0e-8F) {
            continue;
        }
        fromP = fromP.Normalized();
        const Vector3 spot = c + fromP * (r + env.hideAgentRadius + 0.35F);
        const float d2 = (spot - position).LengthSquared();
        if (d2 < bestD2) {
            bestD2 = d2;
            bestSpot = spot;
        }
    }
    const Vector3 dir = SeekDir(position, bestSpot);
    if (dir.LengthSquared() < 1.0e-12F) {
        return Vector3::Zero;
    }
    const Vector3 desired = dir * env.maxSteeringSpeed;
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringPathFollowing3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    if (env.pathPoints == nullptr || env.pathPoints->IsEmpty()) {
        return Vector3::Zero;
    }
    const int n = static_cast<int>(env.pathPoints->GetSize());
    int idx = env.pathIndex;
    if (idx < 0) {
        idx = 0;
    }
    if (idx >= n) {
        idx = n - 1;
    }
    const Vector3 wp = (*env.pathPoints)[static_cast<std::size_t>(idx)];
    const Vector3 dir = SeekDir(position, wp);
    if (dir.LengthSquared() < 1.0e-12F) {
        return Vector3::Zero;
    }
    const Vector3 desired = dir * env.maxSteeringSpeed;
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringOffsetPursuit3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    Vector3 r{};
    Vector3 f{};
    LeaderBasisXZ(env.leaderVelocity, r, f);
    const Vector3 up{0.0F, 1.0F, 0.0F};
    const Vector3 desiredPos =
            env.leaderPosition + r * env.offsetPursuitLocal.x + up * env.offsetPursuitLocal.y +
            f * env.offsetPursuitLocal.z;
    const Vector3 dir = SeekDir(position, desiredPos);
    if (dir.LengthSquared() < 1.0e-12F) {
        return Vector3::Zero;
    }
    const Vector3 desiredVel = dir * env.maxSteeringSpeed;
    return SteerVelocity(desiredVel, velocity, env, weight);
}

Vector3 SteeringSeparation3D::Compute(
        const Vector3& position,
        const Vector3& /*velocity*/,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    if (env.flockPositions == nullptr) {
        return Vector3::Zero;
    }
    Vector3 acc{0.0F, 0.0F, 0.0F};
    const float rad = env.separationRadius > 0.1F ? env.separationRadius : 0.1F;
    for (std::size_t i = 0; i < env.flockPositions->GetSize(); ++i) {
        if (i == env.flockSelfIndex) {
            continue;
        }
        const Vector3 other = (*env.flockPositions)[i];
        Vector3 away = position - other;
        const float d2 = away.LengthSquared();
        if (d2 < 1.0e-8F || d2 > rad * rad) {
            continue;
        }
        const float d = std::sqrt(d2);
        away = away * (1.0F / d);
        acc += away * ((rad - d) / rad);
    }
    return ClampLen(acc, env.maxAcceleration) * weight;
}

Vector3 SteeringAlignment3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    if (env.flockVelocities == nullptr || env.flockPositions == nullptr) {
        return Vector3::Zero;
    }
    Vector3 sum{0.0F, 0.0F, 0.0F};
    int cnt = 0;
    for (std::size_t i = 0; i < env.flockVelocities->GetSize(); ++i) {
        if (i == env.flockSelfIndex) {
            continue;
        }
        if (i >= env.flockPositions->GetSize()) {
            break;
        }
        const float d2 = (position - (*env.flockPositions)[i]).LengthSquared();
        if (d2 > env.cohesionRadius * env.cohesionRadius) {
            continue;
        }
        sum += (*env.flockVelocities)[i];
        ++cnt;
    }
    if (cnt <= 0) {
        return Vector3::Zero;
    }
    sum = sum * (1.0F / static_cast<float>(cnt));
    return ClampLen((sum - velocity) * weight, env.maxAcceleration);
}

Vector3 SteeringCohesion3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& /*board*/) const {
    if (env.flockPositions == nullptr) {
        return Vector3::Zero;
    }
    Vector3 com{0.0F, 0.0F, 0.0F};
    int cnt = 0;
    for (std::size_t i = 0; i < env.flockPositions->GetSize(); ++i) {
        if (i == env.flockSelfIndex) {
            continue;
        }
        const float d2 = (position - (*env.flockPositions)[i]).LengthSquared();
        if (d2 > env.cohesionRadius * env.cohesionRadius) {
            continue;
        }
        com += (*env.flockPositions)[i];
        ++cnt;
    }
    if (cnt <= 0) {
        return Vector3::Zero;
    }
    com = com * (1.0F / static_cast<float>(cnt));
    const Vector3 dir = SeekDir(position, com);
    if (dir.LengthSquared() < 1.0e-12F) {
        return Vector3::Zero;
    }
    const Vector3 desired = dir * env.maxSteeringSpeed;
    return SteerVelocity(desired, velocity, env, weight);
}

Vector3 SteeringFlocking3D::Compute(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& board) const {
    SteeringSeparation3D s(1.0F);
    SteeringAlignment3D a(1.0F);
    SteeringCohesion3D c(1.0F);
    const Vector3 v =
            s.Compute(position, velocity, env, board) * sepW + a.Compute(position, velocity, env, board) * aliW +
            c.Compute(position, velocity, env, board) * cohW;
    return ClampLen(v, env.maxAcceleration) * weight;
}

Vector3 SteeringComposer3D::Compose(
        const Vector3& position,
        const Vector3& velocity,
        const SteeringEnvironment3D& env,
        AiBlackboard& board) const {
    Vector3 acc{0.0F, 0.0F, 0.0F};
    for (std::size_t i = 0; i < behaviors.GetSize(); ++i) {
        acc += behaviors[i]->Compute(position, velocity, env, board) * weights[i];
    }
    return ClampLen(acc, env.maxAcceleration);
}

}  // namespace Spark
