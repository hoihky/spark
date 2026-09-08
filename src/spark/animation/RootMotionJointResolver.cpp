#include "spark/animation/IRootMotionJointResolver.hpp"

#include "spark/animation/Skeleton.hpp"

namespace Spark {

std::uint32_t FixedRootMotionJointResolver::ResolveJointIndex(const Skeleton& skeleton) const {
    const std::uint32_t count = skeleton.GetJointCount();
    if (count == 0) {
        return 0;
    }
    return jointIndex < count ? jointIndex : 0;
}

std::uint32_t PatternRootMotionJointResolver::ResolveJointIndex(const Skeleton& skeleton) const {
    static constexpr const char* kPatterns[] = {"hips", "hip", "pelvis", "root", "b_root"};
    for (const char* pattern : kPatterns) {
        if (const std::int32_t idx = skeleton.FindJointIndexIfNameContains(pattern); idx >= 0) {
            return static_cast<std::uint32_t>(idx);
        }
    }
    return skeleton.GetJointCount() > 0 ? 0U : 0U;
}

}  // namespace Spark
