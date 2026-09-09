#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Transform.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class Skeleton;

/**
 * Analytic two-bone IK (hip → knee → ankle) in skeleton space.
 */
class TwoBoneIkSolver {
public:
    bool Apply(
            const Skeleton& skeleton,
            Array<Transform>& pose,
            std::uint32_t rootJoint,
            std::uint32_t midJoint,
            std::uint32_t endJoint,
            const Vector3& targetWorld,
            const Vector3& poleHintWorld,
            float weight) const;

private:
    [[nodiscard]] Vector3 ExtractTranslation(const Matrix4& matrix) const noexcept;
};

}  // namespace Spark
