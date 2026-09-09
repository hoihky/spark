#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/components/animation/FootIkComponent.hpp"
#include "spark/math/Transform.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

class Skeleton;
class TwoBoneIkSolver;

/**
 * Solves one foot limb (two-bone or single-bone fallback) toward a ground target.
 */
class FootIkLimbSolver {
public:
    explicit FootIkLimbSolver(const TwoBoneIkSolver& twoBoneSolver) : twoBoneSolverRef(twoBoneSolver) {}

    bool Apply(
            const Skeleton& skeleton,
            Array<Transform>& pose,
            const FootIkComponent::Limb& limb,
            const Vector3& targetSkeleton,
            const Vector3& groundNormal,
            const Vector3& poleHintSkeleton,
            float weight) const;

private:
    const TwoBoneIkSolver& twoBoneSolverRef;
};

}  // namespace Spark
