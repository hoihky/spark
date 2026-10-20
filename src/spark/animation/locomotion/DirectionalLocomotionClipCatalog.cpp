#include "spark/animation/locomotion/DirectionalLocomotionClipCatalog.hpp"

#include "spark/animation/Skeleton.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"

namespace Spark {

std::uint32_t DirectionalLocomotionClipCatalog::FindClipOrInvalid(
        const Skeleton& skeleton,
        const char* const* names,
        const std::size_t count) noexcept {
    for (std::size_t i = 0; i < count; ++i) {
        if (const std::int32_t exact = skeleton.FindClipIndexByNameCaseInsensitive(names[i]); exact >= 0) {
            return static_cast<std::uint32_t>(exact);
        }
    }
    for (std::size_t i = 0; i < count; ++i) {
        if (const std::int32_t partial = skeleton.FindClipIndexIfNameContains(names[i]); partial >= 0) {
            return static_cast<std::uint32_t>(partial);
        }
    }
    return kInvalidAnimClipIndex;
}

void DirectionalLocomotionClipCatalog::ResolveFromSkeleton(
        const Skeleton& skeleton,
        const LocomotionClipSet& baseSet) noexcept {
    idle = baseSet.idle;
    forwardWalk = baseSet.walk;
    forwardRun = baseSet.run;

    static constexpr const char* kStrafeLeft[] = {"strafe_left", "left_strafe", "walk_left"};
    strafeLeftWalk = FindClipOrInvalid(skeleton, kStrafeLeft, 3);

    static constexpr const char* kStrafeRight[] = {"strafe_right", "right_strafe", "walk_right"};
    strafeRightWalk = FindClipOrInvalid(skeleton, kStrafeRight, 3);

    static constexpr const char* kBackward[] = {"walk_back", "backward", "back_walk"};
    backwardWalk = FindClipOrInvalid(skeleton, kBackward, 3);

    static constexpr const char* kTurnLeft[] = {"turn_left", "rotate_left"};
    turnLeft = FindClipOrInvalid(skeleton, kTurnLeft, 2);

    static constexpr const char* kTurnRight[] = {"turn_right", "rotate_right"};
    turnRight = FindClipOrInvalid(skeleton, kTurnRight, 2);
}

bool DirectionalLocomotionClipCatalog::HasDirectionalLocomotion(const AnimatorComponent& anim) const noexcept {
    const auto valid = [&](const std::uint32_t clip) {
        return clip != kInvalidAnimClipIndex && clip < anim.GetClipCount();
    };
    return valid(forwardWalk)
            && (valid(strafeLeftWalk) || valid(strafeRightWalk) || valid(backwardWalk));
}

}  // namespace Spark
