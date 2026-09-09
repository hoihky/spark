#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

class Skeleton;

/**
 * Foot placement IK: raycasts down from each foot and solves ankle/knee with two-bone IK when available.
 */
class FootIkComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::FootIk;

    class Limb {
    public:
        std::uint32_t rootJoint = 0;
        std::uint32_t midJoint = 0;
        std::uint32_t endJoint = 0;
        bool hasMidJoint = false;
        float poleBiasX = 0.0F;
        Utf8String endPattern{};
    };

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }
    [[nodiscard]] int UpdatePriority() const noexcept override { return 215; }

    [[nodiscard]] bool IsEnabled() const noexcept { return enabled; }
    [[nodiscard]] float GetWeight() const noexcept { return weight; }
    [[nodiscard]] float GetRayOriginLift() const noexcept { return rayOriginLift; }
    [[nodiscard]] float GetRayMaxDistance() const noexcept { return rayMaxDistance; }
    [[nodiscard]] const Array<Limb>& GetLimbs() const noexcept { return limbs; }

    void SetEnabled(bool value) noexcept { enabled = value; }
    void SetWeight(float value) noexcept { weight = value; }
    void SetRayOriginLift(float value) noexcept { rayOriginLift = value; }
    void SetRayMaxDistance(float value) noexcept { rayMaxDistance = value; }

    /** Resolves left/right foot chains from joint name patterns on the skeleton. */
    bool ConfigureFromSkeleton(const Skeleton& skeleton);

    void SetLeftFootPatterns(const char* rootPattern, const char* midPattern, const char* endPattern);
    void SetRightFootPatterns(const char* rootPattern, const char* midPattern, const char* endPattern);

private:
    static bool TryResolveLimb(
            const Skeleton& skeleton,
            const Utf8String& rootPattern,
            const Utf8String& midPattern,
            const Utf8String& endPattern,
            const Array<std::uint32_t>& excludedEndJoints,
            Limb& outLimb);

    static bool IsJointAncestorOf(const Skeleton& skeleton, std::uint32_t ancestorJoint, std::uint32_t jointIndex);
    static std::int32_t FindJointMatchingPatternOnChainToEnd(
            const Skeleton& skeleton,
            const Utf8String& pattern,
            std::uint32_t endJoint,
            bool preferLastMatch);
    static float ResolvePoleBiasX(const Skeleton& skeleton, std::uint32_t endJoint);
    static bool MatchesJointNamePattern(const char* jointName, const char* pattern);

    bool enabled = true;
    float weight = 1.0F;
    float rayOriginLift = 0.35F;
    float rayMaxDistance = 1.25F;
    Utf8String leftRootPattern{"thigh"};
    Utf8String leftMidPattern{"knee"};
    Utf8String leftEndPattern{"foot"};
    Utf8String rightRootPattern{"thigh"};
    Utf8String rightMidPattern{"knee"};
    Utf8String rightEndPattern{"foot"};
    Array<Limb> limbs{};
};

}  // namespace Spark
