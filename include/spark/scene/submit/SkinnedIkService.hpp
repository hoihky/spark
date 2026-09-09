#pragma once

#include "spark/animation/ik/IkGroundProbe.hpp"
#include "spark/animation/ik/SkinnedIkPipeline.hpp"
#include "spark/core/Array.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Transform.hpp"

namespace Spark {

class AnimatorComponent;
class FootIkComponent;
class AimIkComponent;
class GameObject;
class PhysicsQueryWorld3D;

/**
 * Per-world coordinator for skinned IK (ground probe + pose post-process pipeline).
 */
class SkinnedIkService {
public:
    void SetPhysicsQueryWorld(PhysicsQueryWorld3D* queries) noexcept { groundProbe.SetPhysicsQueryWorld(queries); }
    void SetFallbackGroundHeight(float height) noexcept { groundProbe.SetFallbackPlaneHeight(height); }

    [[nodiscard]] static bool ObjectUsesIk(const GameObject& owner) noexcept;

    bool ApplyToPose(
            GameObject& owner,
            const Matrix4& ownerWorld,
            const AnimatorComponent& animator,
            Array<Transform>& pose);

private:
    IkGroundProbe groundProbe{};
    SkinnedIkPipeline pipeline{};
};

}  // namespace Spark
