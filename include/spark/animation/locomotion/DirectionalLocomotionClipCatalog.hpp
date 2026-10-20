#pragma once

#include "spark/animation/LocomotionClipSet.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

class AnimatorComponent;
class Skeleton;

/** Clip indices for directional locomotion (forward / strafe / back) and optional turn clips. */
class DirectionalLocomotionClipCatalog final {
public:
    void SetForwardWalkClip(const std::uint32_t clip) noexcept { forwardWalk = clip; }
    void SetForwardRunClip(const std::uint32_t clip) noexcept { forwardRun = clip; }
    void SetStrafeLeftWalkClip(const std::uint32_t clip) noexcept { strafeLeftWalk = clip; }
    void SetStrafeRightWalkClip(const std::uint32_t clip) noexcept { strafeRightWalk = clip; }
    void SetBackwardWalkClip(const std::uint32_t clip) noexcept { backwardWalk = clip; }
    void SetTurnLeftClip(const std::uint32_t clip) noexcept { turnLeft = clip; }
    void SetTurnRightClip(const std::uint32_t clip) noexcept { turnRight = clip; }
    void SetIdleClip(const std::uint32_t clip) noexcept { idle = clip; }

    [[nodiscard]] std::uint32_t GetForwardWalkClip() const noexcept { return forwardWalk; }
    [[nodiscard]] std::uint32_t GetForwardRunClip() const noexcept { return forwardRun; }
    [[nodiscard]] std::uint32_t GetStrafeLeftWalkClip() const noexcept { return strafeLeftWalk; }
    [[nodiscard]] std::uint32_t GetStrafeRightWalkClip() const noexcept { return strafeRightWalk; }
    [[nodiscard]] std::uint32_t GetBackwardWalkClip() const noexcept { return backwardWalk; }
    [[nodiscard]] std::uint32_t GetTurnLeftClip() const noexcept { return turnLeft; }
    [[nodiscard]] std::uint32_t GetTurnRightClip() const noexcept { return turnRight; }
    [[nodiscard]] std::uint32_t GetIdleClip() const noexcept { return idle; }

    /** Seeds forward/idle/run from a resolved <c>LocomotionClipSet</c> and searches optional strafe / turn names. */
    void ResolveFromSkeleton(const Skeleton& skeleton, const LocomotionClipSet& baseSet) noexcept;

    [[nodiscard]] bool HasDirectionalLocomotion(const AnimatorComponent& anim) const noexcept;

private:
    [[nodiscard]] static std::uint32_t FindClipOrInvalid(const Skeleton& skeleton, const char* const* names, std::size_t count)
            noexcept;

    std::uint32_t idle = 0;
    std::uint32_t forwardWalk = kInvalidAnimClipIndex;
    std::uint32_t forwardRun = kInvalidAnimClipIndex;
    std::uint32_t strafeLeftWalk = kInvalidAnimClipIndex;
    std::uint32_t strafeRightWalk = kInvalidAnimClipIndex;
    std::uint32_t backwardWalk = kInvalidAnimClipIndex;
    std::uint32_t turnLeft = kInvalidAnimClipIndex;
    std::uint32_t turnRight = kInvalidAnimClipIndex;
};

}  // namespace Spark
