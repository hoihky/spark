#pragma once

#include "spark/ai/steering/ISteeringBehavior.hpp"
#include "spark/core/Array.hpp"

namespace Spark {

/** Chase a point stored in blackboard float slots (0=x, 1=z). */
class SteeringSeek final : public ISteeringBehavior {
public:
    explicit SteeringSeek(const float weight) noexcept : weight(weight) {}

    Vector2 ComputeAcceleration(
            const Vector2& positionXZ,
            const Vector2& velocityXZ,
            AiBlackboard& board) const override;

private:
    float weight = 1.0F;
};

/** Flee from target in blackboard slots (0,1). */
class SteeringFlee final : public ISteeringBehavior {
public:
    explicit SteeringFlee(const float weight) noexcept : weight(weight) {}

    Vector2 ComputeAcceleration(
            const Vector2& positionXZ,
            const Vector2& velocityXZ,
            AiBlackboard& board) const override;

private:
    float weight = 1.0F;
};

/** Arrive with slowing radius; target (0,1), slowing radius in float slot 2. */
class SteeringArrive final : public ISteeringBehavior {
public:
    explicit SteeringArrive(const float weight) noexcept : weight(weight) {}

    Vector2 ComputeAcceleration(
            const Vector2& positionXZ,
            const Vector2& velocityXZ,
            AiBlackboard& board) const override;

private:
    float weight = 1.0F;
};

/** Random walk using blackboard int slot 0 as RNG seed scratch. */
class SteeringWander final : public ISteeringBehavior {
public:
    explicit SteeringWander(const float weight, const float wanderStrength) noexcept
            : weight(weight), wanderStrength(wanderStrength) {}

    Vector2 ComputeAcceleration(
            const Vector2& positionXZ,
            const Vector2& velocityXZ,
            AiBlackboard& board) const override;

private:
    float weight = 1.0F;
    float wanderStrength = 0.8F;
};

/**
 * Weighted sum of behaviors (Open/Closed: add behaviors without modifying composer).
 */
class SteeringComposer final {
public:
    void Clear() noexcept { behaviors.Clear(); weights.Clear(); }
    void AddBehavior(const ISteeringBehavior& behavior, const float weight) {
        behaviors.PushBack(&behavior);
        weights.PushBack(weight);
    }

    [[nodiscard]] Vector2 Compose(
            const Vector2& positionXZ,
            const Vector2& velocityXZ,
            AiBlackboard& board) const;

private:
    Array<const ISteeringBehavior*> behaviors;
    Array<float> weights;
};

}  // namespace Spark
