#include "spark/animation/locomotion/CardinalDirectionMixer.hpp"

#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/animation/locomotion/DirectionalLocomotionClipCatalog.hpp"

#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] float SmoothStep01(const float t) noexcept {
    const float x = (t < 0.0F) ? 0.0F : (t > 1.0F ? 1.0F : t);
    return x * x * (3.0F - 2.0F * x);
}

}  // namespace

std::uint32_t CardinalDirectionMixer::PickGaitClip(
        const std::uint32_t walkClip,
        const std::uint32_t runClip,
        const float planarSpeed,
        const float walkSpeedThreshold,
        const float runSpeedThreshold,
        const bool wantSprint) const noexcept {
    if (wantSprint && runClip != kInvalidAnimClipIndex && planarSpeed >= walkSpeedThreshold) {
        return runClip;
    }
    if (walkClip != kInvalidAnimClipIndex) {
        return walkClip;
    }
    if (runClip != kInvalidAnimClipIndex && planarSpeed >= runSpeedThreshold) {
        return runClip;
    }
    return walkClip;
}

void CardinalDirectionMixer::Evaluate(
        const float signedMoveAngleRadians,
        const float planarSpeed,
        const float walkSpeedThreshold,
        const float runSpeedThreshold,
        const bool wantSprint,
        std::uint32_t& outClipA,
        std::uint32_t& outClipB,
        float& outBlend01) const noexcept {
    if (catalog == nullptr) {
        outClipA = 0;
        outClipB = 0;
        outBlend01 = 0.0F;
        return;
    }

    const std::uint32_t forward = PickGaitClip(
            catalog->GetForwardWalkClip(),
            catalog->GetForwardRunClip(),
            planarSpeed,
            walkSpeedThreshold,
            runSpeedThreshold,
            wantSprint);
    const std::uint32_t backward = PickGaitClip(
            catalog->GetBackwardWalkClip(),
            kInvalidAnimClipIndex,
            planarSpeed,
            walkSpeedThreshold,
            runSpeedThreshold,
            false);
    const std::uint32_t left = PickGaitClip(
            catalog->GetStrafeLeftWalkClip(),
            kInvalidAnimClipIndex,
            planarSpeed,
            walkSpeedThreshold,
            runSpeedThreshold,
            false);
    const std::uint32_t right = PickGaitClip(
            catalog->GetStrafeRightWalkClip(),
            kInvalidAnimClipIndex,
            planarSpeed,
            walkSpeedThreshold,
            runSpeedThreshold,
            false);

    constexpr float kQuarter = 0.7853981633974483F;
    const float angle = signedMoveAngleRadians;

    if (angle >= -kQuarter && angle <= kQuarter) {
        outClipA = forward;
        outClipB = right != kInvalidAnimClipIndex ? right : forward;
        outBlend01 = (right != kInvalidAnimClipIndex) ? SmoothStep01((angle + kQuarter) / (2.0F * kQuarter)) : 0.0F;
        return;
    }
    if (angle > kQuarter && angle <= 3.0F * kQuarter) {
        outClipA = right != kInvalidAnimClipIndex ? right : forward;
        outClipB = backward != kInvalidAnimClipIndex ? backward : outClipA;
        outBlend01 = SmoothStep01((angle - kQuarter) / (2.0F * kQuarter));
        return;
    }
    if (angle < -kQuarter && angle >= -3.0F * kQuarter) {
        outClipA = left != kInvalidAnimClipIndex ? left : forward;
        outClipB = backward != kInvalidAnimClipIndex ? backward : outClipA;
        outBlend01 = SmoothStep01((-angle - kQuarter) / (2.0F * kQuarter));
        return;
    }
    outClipA = backward != kInvalidAnimClipIndex ? backward : forward;
    outClipB = left != kInvalidAnimClipIndex ? left : outClipA;
    outBlend01 = (left != kInvalidAnimClipIndex) ? 0.35F : 0.0F;
}

}  // namespace Spark
