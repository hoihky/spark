#include "spark/animation/AnimationJointMask.hpp"

#include "spark/animation/Skeleton.hpp"

namespace Spark {

void AnimationJointMask::MarkSubtree_(const Skeleton& skeleton, const std::uint32_t jointIndex) noexcept {
    const std::uint32_t jointCount = skeleton.GetJointCount();
    if (jointIndex >= jointCount || weights.GetSize() < jointCount) {
        return;
    }
    weights[jointIndex] = 1.0F;
    for (std::uint32_t child = 0; child < jointCount; ++child) {
        if (static_cast<std::uint32_t>(skeleton.GetJointParent(child)) == jointIndex) {
            MarkSubtree_(skeleton, child);
        }
    }
}

void AnimationJointMask::BuildUpperBodySubtree(const Skeleton& skeleton, const std::int32_t rootJointIndex) noexcept {
    weights.Clear();
    const std::uint32_t jointCount = skeleton.GetJointCount();
    if (jointCount == 0 || rootJointIndex < 0) {
        return;
    }
    weights.Reserve(jointCount);
    for (std::uint32_t i = 0; i < jointCount; ++i) {
        weights.PushBack(0.0F);
    }
    MarkSubtree_(skeleton, static_cast<std::uint32_t>(rootJointIndex));
}

bool AnimationJointMask::TryBuildUpperBodyFromSpineName(
        const Skeleton& skeleton,
        const char* spineNameHint) noexcept {
    const std::int32_t spine = skeleton.FindJointIndexIfNameContains(spineNameHint);
    if (spine < 0) {
        return false;
    }
    BuildUpperBodySubtree(skeleton, spine);
    return true;
}

}  // namespace Spark
