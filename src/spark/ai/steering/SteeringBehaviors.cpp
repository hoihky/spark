#include "spark/ai/steering/SteeringBehaviors.hpp"

#include "spark/ai/AiBlackboard.hpp"

#include <cmath>

namespace Spark {

Vector2 SteeringSeek::ComputeAcceleration(
        const Vector2& positionXZ,
        const Vector2& /*velocityXZ*/,
        AiBlackboard& board) const {
    const Vector2 target{board.GetFloat(0), board.GetFloat(1)};
    Vector2 desired = target - positionXZ;
    if (desired.LengthSquared() < 1.0e-8F) {
        return Vector2::Zero;
    }
    desired = desired.Normalized() * weight;
    return desired;
}

Vector2 SteeringFlee::ComputeAcceleration(
        const Vector2& positionXZ,
        const Vector2& /*velocityXZ*/,
        AiBlackboard& board) const {
    const Vector2 target{board.GetFloat(0), board.GetFloat(1)};
    Vector2 away = positionXZ - target;
    if (away.LengthSquared() < 1.0e-8F) {
        return Vector2::Zero;
    }
    return away.Normalized() * weight;
}

Vector2 SteeringArrive::ComputeAcceleration(
        const Vector2& positionXZ,
        const Vector2& velocityXZ,
        AiBlackboard& board) const {
    const Vector2 target{board.GetFloat(0), board.GetFloat(1)};
    const float br = board.GetFloat(2);
    const float slowRadius = br > 0.05F ? br : 0.05F;
    Vector2 offset = target - positionXZ;
    const float dist = offset.Length();
    if (dist < 1.0e-5F) {
        return velocityXZ * (-1.0F) * weight;
    }
    float speed = 1.0F;
    if (dist < slowRadius) {
        speed = dist / slowRadius;
    }
    Vector2 desired = offset.Normalized() * speed;
    return (desired - velocityXZ) * weight;
}

Vector2 SteeringWander::ComputeAcceleration(
        const Vector2& /*positionXZ*/,
        const Vector2& velocityXZ,
        AiBlackboard& board) const {
    int seed = board.GetInt(0);
    seed = seed * 1103515245 + 12345;
    board.SetInt(0, seed);
    const float u1 = static_cast<float>((seed >> 8) & 0xffff) / 65535.0F;
    const float u2 = static_cast<float>((seed >> 20) & 0xffff) / 65535.0F;
    const float twoPi = 6.2831855F;
    const float angle = (u1 + u2) * 0.5F * twoPi;
    Vector2 dir{std::cos(angle), std::sin(angle)};
    if (velocityXZ.LengthSquared() > 1.0e-6F) {
        const Vector2 base = velocityXZ.Normalized();
        dir = Vector2{base.x * dir.x - base.y * dir.y, base.x * dir.y + base.y * dir.x};
    }
    return dir * (wanderStrength * weight);
}

Vector2 SteeringComposer::Compose(
        const Vector2& positionXZ,
        const Vector2& velocityXZ,
        AiBlackboard& board) const {
    Vector2 acc{0.0F, 0.0F};
    for (std::size_t i = 0; i < behaviors.GetSize(); ++i) {
        acc += behaviors[i]->ComputeAcceleration(positionXZ, velocityXZ, board) * weights[i];
    }
    return acc;
}

}  // namespace Spark
