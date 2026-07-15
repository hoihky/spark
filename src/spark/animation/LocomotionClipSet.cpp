#include "spark/animation/LocomotionClipSet.hpp"

#include "spark/animation/Skeleton.hpp"

namespace Spark {

namespace {

std::int32_t FindFirstLocomotionClip(const Skeleton& sk, const char* const* names, const std::size_t nameCount) {
    for (std::size_t ni = 0; ni < nameCount; ++ni) {
        if (const std::int32_t exact = sk.FindClipIndexByNameCaseInsensitive(names[ni]); exact >= 0) {
            return exact;
        }
    }
    for (std::size_t ni = 0; ni < nameCount; ++ni) {
        if (const std::int32_t partial = sk.FindClipIndexIfNameContains(names[ni]); partial >= 0) {
            return partial;
        }
    }
    return -1;
}

}  // namespace

LocomotionClipSet ResolveLocomotionClipsFromSkeleton(
        const Skeleton& skeleton,
        const std::uint32_t walkFallback) noexcept {
    LocomotionClipSet out{};
    out.walk = walkFallback;

    static constexpr const char* kIdleNames[] = {"idle", "survey", "rest", "stand"};
    if (const std::int32_t idleIdx = FindFirstLocomotionClip(skeleton, kIdleNames, 4); idleIdx >= 0) {
        out.idle = static_cast<std::uint32_t>(idleIdx);
    }

    static constexpr const char* kWalkNames[] = {"walk"};
    if (const std::int32_t walkIdx = FindFirstLocomotionClip(skeleton, kWalkNames, 1); walkIdx >= 0) {
        out.walk = static_cast<std::uint32_t>(walkIdx);
    }

    static constexpr const char* kRunNames[] = {"run", "jog", "sprint"};
    if (const std::int32_t runIdx = FindFirstLocomotionClip(skeleton, kRunNames, 3); runIdx >= 0) {
        out.run = static_cast<std::uint32_t>(runIdx);
    }

    static constexpr const char* kAttackNames[] = {"attack", "punch", "kick", "shoot"};
    if (const std::int32_t attackIdx = FindFirstLocomotionClip(skeleton, kAttackNames, 4); attackIdx >= 0) {
        out.attack = static_cast<std::uint32_t>(attackIdx);
    }

    return out;
}

}  // namespace Spark
