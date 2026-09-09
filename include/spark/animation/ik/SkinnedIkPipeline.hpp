#pragma once

#include "spark/animation/ik/FootIkLimbSolver.hpp"
#include "spark/animation/ik/IkGroundProbe.hpp"
#include "spark/animation/ik/SpineAimIkSolver.hpp"
#include "spark/animation/ik/TwoBoneIkSolver.hpp"
#include "spark/core/Array.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Transform.hpp"

namespace Spark {

class AimIkComponent;
class AnimatorComponent;
class FootIkComponent;
class GameObject;
class GameWorld;

/**
 * Post-animation IK pipeline (foot placement + spine aim) applied to a sampled pose.
 */
class SkinnedIkPipeline {
public:
    void SetGroundProbe(const IkGroundProbe& probe) noexcept { groundProbe = &probe; }

    bool Apply(
            GameObject& owner,
            const Matrix4& ownerWorld,
            const AnimatorComponent& animator,
            Array<Transform>& pose) const;

private:
    const IkGroundProbe* groundProbe = nullptr;
    TwoBoneIkSolver twoBoneSolver{};
    SpineAimIkSolver spineAimSolver{};
    FootIkLimbSolver footLimbSolver{twoBoneSolver};

    void ApplyFootIk(
            GameObject& owner,
            const Matrix4& ownerWorld,
            const Skeleton& skeleton,
            const FootIkComponent& footIk,
            Array<Transform>& pose) const;

    void ApplyAimIk(
            GameObject& owner,
            const Matrix4& ownerWorld,
            const Skeleton& skeleton,
            const AimIkComponent& aimIk,
            Array<Transform>& pose) const;

    [[nodiscard]] Vector3 ToWorldPosition(const Matrix4& ownerWorld, const Vector3& skeletonPosition) const noexcept;
    [[nodiscard]] Vector3 ToSkeletonPosition(const Matrix4& ownerWorld, const Vector3& worldPosition) const;
};

}  // namespace Spark
