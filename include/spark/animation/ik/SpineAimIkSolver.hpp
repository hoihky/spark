#pragma once

#include "spark/core/Array.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Transform.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class Skeleton;

/**
 * Partial spine-chain rotation toward a world-space aim target.
 */
class SpineAimIkSolver {
public:
    void ApplyChain(
            const Skeleton& skeleton,
            Array<Transform>& pose,
            const Array<std::uint32_t>& jointIndices,
            const Vector3& targetWorld,
            float weight) const;

private:
    [[nodiscard]] Vector3 ExtractBoneForward(const Matrix4& jointWorld) const noexcept;
    void RotateJointToward(
            Transform& jointLocal,
            const Matrix4& parentWorld,
            const Vector3& jointWorldPosition,
            const Vector3& targetWorld,
            float jointWeight) const;
};

}  // namespace Spark
