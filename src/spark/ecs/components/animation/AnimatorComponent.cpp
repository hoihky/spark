#include "spark/ecs/components/animation/AnimatorComponent.hpp"

#include "spark/animation/AnimTime.hpp"
#include "spark/core/Utility.hpp"

#include <cmath>

namespace Spark {

AnimatorComponent::AnimatorComponent(SharedPtr<Skeleton> inSkeleton, std::uint32_t inClip, float inSpeed)
    : skeleton(MoveTemp(inSkeleton)), clipIndex(inClip), speed(inSpeed) {}

void AnimatorComponent::OnUpdate(const FrameTiming& timing, GameObject& /*owner*/, IEngineContext& /*context*/) {
    if (!skeleton || skeleton->GetClipCount() == 0) {
        return;
    }
    AdvanceCrossfade_(timing.deltaTimeSeconds);
    AdvancePrimaryTime_(timing.deltaTimeSeconds);
}

void AnimatorComponent::AdvancePrimaryTime_(const float deltaSeconds) {
    const std::uint32_t c = clipIndex < skeleton->GetClipCount() ? clipIndex : 0;
    const float dur = skeleton->GetClipDuration(c);
    AdvanceAnimClipTime(deltaSeconds, speed, dur, loopMode, timeSeconds, clipFinished);
}

void AnimatorComponent::AdvanceCrossfade_(const float deltaSeconds) {
    if (!crossfade.active) {
        return;
    }
    crossfade.elapsed += deltaSeconds;
    if (crossfade.duration <= 1.0e-4F || crossfade.elapsed >= crossfade.duration) {
        crossfade.active = false;
    }
}

std::uint32_t AnimatorComponent::GetClipCount() const noexcept {
    return skeleton ? skeleton->GetClipCount() : 0;
}

const Utf8String& AnimatorComponent::GetClipName(const std::uint32_t index) const {
    static const Utf8String kEmpty{};
    if (!skeleton) {
        return kEmpty;
    }
    return skeleton->GetClipName(index);
}

std::int32_t AnimatorComponent::FindClipIndexByName(const char* name) const {
    if (!skeleton) {
        return -1;
    }
    return skeleton->FindClipIndexByName(name);
}

void AnimatorComponent::SetClipIndex(const std::uint32_t c) {
    if (c == clipIndex && !locomotionBlend.active) {
        return;
    }
    clipIndex = c;
    timeSeconds = 0.0F;
    clipFinished = false;
    crossfade.active = false;
    locomotionBlend.active = false;
}

void AnimatorComponent::SetClipIndexWithCrossfade(const std::uint32_t c, const float crossfadeDurationSec) {
    if (!skeleton || c >= skeleton->GetClipCount()) {
        SetClipIndex(c);
        return;
    }
    if (c == clipIndex && !crossfade.active && !locomotionBlend.active) {
        return;
    }
    crossfade.active = true;
    crossfade.fromClip = clipIndex < skeleton->GetClipCount() ? clipIndex : 0;
    if (locomotionBlend.active) {
        crossfade.fromClip = locomotionBlend.blend01 >= 0.5F ? locomotionBlend.clipB : locomotionBlend.clipA;
    }
    crossfade.fromTime = timeSeconds;
    crossfade.duration = (crossfadeDurationSec > 1.0e-4F) ? crossfadeDurationSec : 0.2F;
    crossfade.elapsed = 0.0F;
    clipIndex = c;
    timeSeconds = 0.0F;
    clipFinished = false;
    locomotionBlend.active = false;
}

void AnimatorComponent::SetSpeed(const float s) {
    speed = s;
}

void AnimatorComponent::SetTimeSeconds(const float t) {
    timeSeconds = t;
    clipFinished = false;
}

void AnimatorComponent::SetLoopMode(const AnimLoopMode mode) noexcept {
    loopMode = mode;
    if (loopMode == AnimLoopMode::Loop) {
        clipFinished = false;
    }
}

void AnimatorComponent::RestartCurrentClip() noexcept {
    timeSeconds = 0.0F;
    clipFinished = false;
    crossfade.active = false;
    locomotionBlend.active = false;
}

void AnimatorComponent::RetargetSkeleton(
        SharedPtr<Skeleton> newSkeleton,
        const std::uint32_t newClipIndex,
        const float newSpeed) {
    skeleton = MoveTemp(newSkeleton);
    const std::uint32_t clipCount = GetClipCount();
    clipIndex = clipCount > 0 ? (newClipIndex < clipCount ? newClipIndex : 0U) : 0U;
    speed = newSpeed;
    timeSeconds = 0.0F;
    clipFinished = false;
    loopMode = AnimLoopMode::Loop;
    crossfade.active = false;
    locomotionBlend.active = false;
}

void AnimatorComponent::SetLocomotionBlend(
        const std::uint32_t clipA,
        const std::uint32_t clipB,
        const float blend01) {
    if (!skeleton || skeleton->GetClipCount() == 0) {
        return;
    }
    const std::uint32_t count = skeleton->GetClipCount();
    const std::uint32_t safeA = clipA < count ? clipA : 0U;
    const std::uint32_t safeB = clipB < count ? clipB : safeA;
    const float clamped = (blend01 < 0.0F) ? 0.0F : (blend01 > 1.0F ? 1.0F : blend01);

    locomotionBlend.active = true;
    locomotionBlend.clipA = safeA;
    locomotionBlend.clipB = safeB;
    locomotionBlend.blend01 = clamped;
    clipIndex = clamped >= 0.5F ? safeB : safeA;
    crossfade.active = false;
    loopMode = AnimLoopMode::Loop;
    clipFinished = false;
}

void AnimatorComponent::ClearLocomotionBlend() noexcept {
    locomotionBlend.active = false;
}

void AnimatorComponent::ComputeJointPalette(Matrix4* outPalette, const std::uint32_t paletteMax) const {
    if (outPalette == nullptr || !skeleton || skeleton->GetJointCount() == 0) {
        return;
    }

    if (crossfade.active && crossfade.duration > 1.0e-4F) {
        const std::uint32_t c = clipIndex < skeleton->GetClipCount() ? clipIndex : 0;
        const float dur = skeleton->GetClipDuration(c);
        const float sampleT = EvaluateAnimSampleTime(timeSeconds, dur, loopMode);
        const float blend = crossfade.elapsed / crossfade.duration;
        const float fromDur = skeleton->GetClipDuration(crossfade.fromClip);
        const float fromSampleT = EvaluateAnimSampleTime(crossfade.fromTime, fromDur, AnimLoopMode::Hold);
        skeleton->ComputeBlendedPalette(
                crossfade.fromClip,
                fromSampleT,
                c,
                sampleT,
                blend,
                outPalette,
                paletteMax);
        return;
    }

    if (locomotionBlend.active) {
        const float sampleA = EvaluateAnimSampleTime(
                timeSeconds,
                skeleton->GetClipDuration(locomotionBlend.clipA),
                AnimLoopMode::Loop);
        const float sampleB = EvaluateAnimSampleTime(
                timeSeconds,
                skeleton->GetClipDuration(locomotionBlend.clipB),
                AnimLoopMode::Loop);
        skeleton->ComputeBlendedPalette(
                locomotionBlend.clipA,
                sampleA,
                locomotionBlend.clipB,
                sampleB,
                locomotionBlend.blend01,
                outPalette,
                paletteMax);
        return;
    }

    const std::uint32_t c = clipIndex < skeleton->GetClipCount() ? clipIndex : 0;
    const float dur = skeleton->GetClipDuration(c);
    const float sampleT = EvaluateAnimSampleTime(timeSeconds, dur, loopMode);

    Array<Transform> pose;
    skeleton->SampleClipPose(c, sampleT, pose);
    skeleton->BuildPaletteFromPose(pose, outPalette, paletteMax);
}

}  // namespace Spark
