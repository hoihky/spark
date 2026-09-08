#include "spark/animation/RootMotionSampler.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/AnimTime.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/math/Matrix4.hpp"

namespace Spark {

namespace {

[[nodiscard]] Vector3 LerpVec3(const Vector3& a, const Vector3& b, const float t) noexcept {
    const float u = 1.0F - t;
    return {a.x * u + b.x * t, a.y * u + b.y * t, a.z * u + b.z * t};
}

[[nodiscard]] std::uint32_t PrimaryClipIndex(const AnimatorComponent& animator) noexcept {
    if (animator.IsLocomotionBlending()) {
        return animator.GetLocomotionBlend01() >= 0.5F ? animator.GetLocomotionBlendClipB()
                                                        : animator.GetLocomotionBlendClipA();
    }
    return animator.GetClipIndex();
}

}  // namespace

bool RootMotionSampler::TrySampleJointPositionAtTime(
        const Skeleton& skeleton,
        const std::uint32_t jointIndex,
        const std::uint32_t clipIndex,
        const float timeSeconds,
        const AnimLoopMode loopMode,
        Vector3& outPosition) const {
    if (jointIndex >= skeleton.GetJointCount() || clipIndex >= skeleton.GetClipCount()) {
        return false;
    }
    const float duration = skeleton.GetClipDuration(clipIndex);
    const float sampleTime = EvaluateAnimSampleTime(timeSeconds, duration, loopMode);
    Matrix4 jointWorld{};
    if (!skeleton.TryComputeJointWorldMatrix(clipIndex, sampleTime, jointIndex, jointWorld)) {
        return false;
    }
    outPosition = jointWorld.TranslationVector();
    return true;
}

bool RootMotionSampler::TrySampleBlendedPosition(
        const Skeleton& skeleton,
        const std::uint32_t jointIndex,
        const std::uint32_t clipA,
        const float timeA,
        const std::uint32_t clipB,
        const float timeB,
        const float blendB,
        Vector3& outPosition) const {
    Vector3 posA{};
    Vector3 posB{};
    if (!TrySampleJointPositionAtTime(skeleton, jointIndex, clipA, timeA, AnimLoopMode::Loop, posA)) {
        return false;
    }
    if (!TrySampleJointPositionAtTime(skeleton, jointIndex, clipB, timeB, AnimLoopMode::Loop, posB)) {
        return false;
    }
    outPosition = LerpVec3(posA, posB, blendB);
    return true;
}

bool RootMotionSampler::TrySampleMotionJointPosition(
        const AnimatorComponent& animator,
        const std::uint32_t jointIndex,
        Vector3& outPosition) const {
    const SharedPtr<Skeleton>& skeleton = animator.GetSkeleton();
    if (!skeleton || jointIndex >= skeleton->GetJointCount()) {
        return false;
    }

    if (animator.IsLocomotionBlending()) {
        return TrySampleBlendedPosition(
                *skeleton,
                jointIndex,
                animator.GetLocomotionBlendClipA(),
                animator.GetTimeSeconds(),
                animator.GetLocomotionBlendClipB(),
                animator.GetTimeSeconds(),
                animator.GetLocomotionBlend01(),
                outPosition);
    }

    if (animator.IsCrossfading()) {
        const float blend = animator.GetCrossfadeBlend01();
        Vector3 fromPos{};
        Vector3 toPos{};
        if (!TrySampleJointPositionAtTime(
                    *skeleton,
                    jointIndex,
                    animator.GetCrossfadeFromClip(),
                    animator.GetCrossfadeFromTime(),
                    AnimLoopMode::Hold,
                    fromPos)) {
            return false;
        }
        if (!TrySampleJointPositionAtTime(
                    *skeleton,
                    jointIndex,
                    animator.GetClipIndex(),
                    animator.GetTimeSeconds(),
                    animator.GetLoopMode(),
                    toPos)) {
            return false;
        }
        outPosition = LerpVec3(fromPos, toPos, blend);
        return true;
    }

    return TrySampleJointPositionAtTime(
            *skeleton,
            jointIndex,
            animator.GetClipIndex(),
            animator.GetTimeSeconds(),
            animator.GetLoopMode(),
            outPosition);
}

bool RootMotionSampler::TryComputeDelta(
        const AnimatorComponent& animator,
        const std::uint32_t jointIndex,
        const float deltaSeconds,
        Vector3& inOutPreviousPosition,
        bool& inOutHasPreviousSample,
        RootMotionDelta& outDelta) const {
    outDelta.valid = false;
    outDelta.translation = Vector3::Zero;

    const SharedPtr<Skeleton>& skeleton = animator.GetSkeleton();
    if (!skeleton || deltaSeconds <= 1.0e-6F) {
        return false;
    }

    Vector3 current{};
    if (!TrySampleMotionJointPosition(animator, jointIndex, current)) {
        inOutHasPreviousSample = false;
        return false;
    }

    if (!inOutHasPreviousSample) {
        inOutPreviousPosition = current;
        inOutHasPreviousSample = true;
        return true;
    }

    const AnimLoopMode loopMode = animator.GetLoopMode();
    const std::uint32_t clipIndex = PrimaryClipIndex(animator);
    const float duration = skeleton->GetClipDuration(clipIndex);
    const float speed = animator.GetSpeed();
    const float currentTime = animator.GetTimeSeconds();
    const float previousTime = currentTime - deltaSeconds * speed;

    Vector3 delta{Vector3::Zero};
    if (loopMode == AnimLoopMode::Loop && duration > 1.0e-4F && previousTime < 0.0F) {
        Vector3 posPrevious{};
        Vector3 posCurrent{};
        const float wrappedPrevious = previousTime + duration;
        if (!TrySampleJointPositionAtTime(
                    *skeleton, jointIndex, clipIndex, wrappedPrevious, loopMode, posPrevious)) {
            return false;
        }
        if (!TrySampleJointPositionAtTime(
                    *skeleton, jointIndex, clipIndex, currentTime, loopMode, posCurrent)) {
            return false;
        }
        delta = posCurrent - posPrevious;
    } else if (loopMode == AnimLoopMode::Loop && duration > 1.0e-4F && currentTime < previousTime) {
        Vector3 posPrevious{};
        Vector3 posEnd{};
        Vector3 posStart{};
        Vector3 posCurrent{};
        if (!TrySampleJointPositionAtTime(
                    *skeleton, jointIndex, clipIndex, previousTime, loopMode, posPrevious)) {
            return false;
        }
        if (!TrySampleJointPositionAtTime(
                    *skeleton, jointIndex, clipIndex, duration, loopMode, posEnd)) {
            return false;
        }
        if (!TrySampleJointPositionAtTime(
                    *skeleton, jointIndex, clipIndex, 0.0F, loopMode, posStart)) {
            return false;
        }
        if (!TrySampleJointPositionAtTime(
                    *skeleton, jointIndex, clipIndex, currentTime, loopMode, posCurrent)) {
            return false;
        }
        delta = (posEnd - posPrevious) + (posCurrent - posStart);
    } else {
        delta = current - inOutPreviousPosition;
    }

    inOutPreviousPosition = current;
    outDelta.translation = delta;
    outDelta.valid = true;
    return true;
}

}  // namespace Spark
