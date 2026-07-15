#pragma once

#include "spark/ai/steering/ISteeringBehavior3D.hpp"
#include "spark/core/Array.hpp"

namespace Spark {

class AiBlackboard;

#define SPARK_STEERING3D_BEHAVIOR(ClassName)                                                                           \
    class ClassName final : public ISteeringBehavior3D {                                                             \
    public:                                                                                                          \
        explicit ClassName(const float weight) noexcept : weight(weight) {}                                         \
        Vector3 Compute(                                                                                             \
                const Vector3& position,                                                                             \
                const Vector3& velocity,                                                                             \
                const SteeringEnvironment3D& env,                                                                  \
                AiBlackboard& board) const override;                                                                 \
                                                                                                                     \
    private:                                                                                                         \
        float weight = 1.0F;                                                                                        \
    }

SPARK_STEERING3D_BEHAVIOR(SteeringSeek3D);
SPARK_STEERING3D_BEHAVIOR(SteeringFlee3D);
SPARK_STEERING3D_BEHAVIOR(SteeringArrive3D);
SPARK_STEERING3D_BEHAVIOR(SteeringPursuit3D);
SPARK_STEERING3D_BEHAVIOR(SteeringEvade3D);
SPARK_STEERING3D_BEHAVIOR(SteeringWander3D);
SPARK_STEERING3D_BEHAVIOR(SteeringObstacleAvoidance3D);
SPARK_STEERING3D_BEHAVIOR(SteeringWallAvoidance3D);
SPARK_STEERING3D_BEHAVIOR(SteeringInterpose3D);
SPARK_STEERING3D_BEHAVIOR(SteeringHide3D);
SPARK_STEERING3D_BEHAVIOR(SteeringPathFollowing3D);
SPARK_STEERING3D_BEHAVIOR(SteeringOffsetPursuit3D);
SPARK_STEERING3D_BEHAVIOR(SteeringSeparation3D);
SPARK_STEERING3D_BEHAVIOR(SteeringAlignment3D);
SPARK_STEERING3D_BEHAVIOR(SteeringCohesion3D);

#undef SPARK_STEERING3D_BEHAVIOR

/** Weighted blend of separation, alignment, cohesion (typical flock). */
class SteeringFlocking3D final : public ISteeringBehavior3D {
public:
    SteeringFlocking3D(const float weight, const float sepW, const float aliW, const float cohW) noexcept
            : weight(weight), sepW(sepW), aliW(aliW), cohW(cohW) {}

    Vector3 Compute(
            const Vector3& position,
            const Vector3& velocity,
            const SteeringEnvironment3D& env,
            AiBlackboard& board) const override;

private:
    float weight = 1.0F;
    float sepW = 1.2F;
    float aliW = 0.8F;
    float cohW = 0.6F;
};

class SteeringComposer3D final {
public:
    void Clear() noexcept {
        behaviors.Clear();
        weights.Clear();
    }
    void AddBehavior(const ISteeringBehavior3D& behavior, const float weight) noexcept {
        behaviors.PushBack(&behavior);
        weights.PushBack(weight);
    }

    [[nodiscard]] Vector3 Compose(
            const Vector3& position,
            const Vector3& velocity,
            const SteeringEnvironment3D& env,
            AiBlackboard& board) const;

private:
    Array<const ISteeringBehavior3D*> behaviors;
    Array<float> weights;
};

}  // namespace Spark
