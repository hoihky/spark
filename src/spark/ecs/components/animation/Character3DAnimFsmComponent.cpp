#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

void Character3DAnimFsmComponent::SetLocomotionClips(
        const std::uint32_t idleClipIndex,
        const std::uint32_t walkClipIndex,
        const std::uint32_t runClipIndex) noexcept {
    idleClip = idleClipIndex;
    walkClip = walkClipIndex;
    runClip = runClipIndex;
}

void Character3DAnimFsmComponent::ConfigureLocomotionFromSkeleton(
        const Skeleton& skeleton,
        const std::uint32_t walkClipFallback) noexcept {
    const LocomotionClipSet clips = ResolveLocomotionClipsFromSkeleton(skeleton, walkClipFallback);
    SetLocomotionClips(clips.idle, clips.walk, clips.run);
    SetAttackClip(clips.attack);
}

void Character3DAnimFsmComponent::SetAttackClip(const std::uint32_t attackClipIndex) noexcept {
    attackClip = attackClipIndex;
}

void Character3DAnimFsmComponent::SetWalkSpeedThreshold(const float metersPerSecond) noexcept {
    walkThresh = (metersPerSecond > 1.0e-6F) ? metersPerSecond : 0.35F;
}

void Character3DAnimFsmComponent::SetRunSpeedThreshold(const float metersPerSecond) noexcept {
    runThresh = (metersPerSecond > walkThresh) ? metersPerSecond : walkThresh + 0.5F;
}

void Character3DAnimFsmComponent::SetCrossfadeDuration(const float seconds) noexcept {
    crossfadeDuration = (seconds > 1.0e-4F) ? seconds : 0.18F;
}

void Character3DAnimFsmComponent::RequestAttack() noexcept {
    pendingAttack = true;
}

void Character3DAnimFsmComponent::SetManualClip(const std::uint32_t clipIndex, const AnimLoopMode loopMode) noexcept {
    manualClipActive = true;
    manualClip = clipIndex;
    manualLoop = loopMode;
    locomotionDrivingEnabled = false;
}

void Character3DAnimFsmComponent::ClearManualClip() noexcept {
    manualClipActive = false;
    locomotionDrivingEnabled = true;
}

void Character3DAnimFsmComponent::SetLocomotionInput(const bool moving, const bool sprint) noexcept {
    hasLocomotionInput = true;
    inputMoving = moving;
    inputSprint = sprint && moving;
}

bool Character3DAnimFsmComponent::ClipValid_(const AnimatorComponent& anim, const std::uint32_t clip) const noexcept {
    return clip < anim.GetClipCount();
}

void Character3DAnimFsmComponent::ApplyClip_(
        AnimatorComponent& anim,
        const std::uint32_t want,
        const AnimLoopMode loopMode) noexcept {
    if (!ClipValid_(anim, want)) {
        return;
    }
    anim.SetLoopMode(loopMode);
    if (want == anim.GetClipIndex() && !anim.IsCrossfading()) {
        return;
    }
    if (loopMode == AnimLoopMode::Once) {
        anim.SetClipIndexWithCrossfade(want, crossfadeDuration);
    } else if (anim.GetClipIndex() != want) {
        anim.SetClipIndexWithCrossfade(want, crossfadeDuration);
    }
}

float Character3DAnimFsmComponent::MeasureLocomotionSpeed_(
        const GameObject& owner,
        const FrameTiming& timing) noexcept {
    if (const Rigidbody3DComponent* rb = owner.GetComponent<Rigidbody3DComponent>()) {
        const Vector3 v = rb->GetVelocity();
        return std::sqrt(v.x * v.x + v.z * v.z);
    }
    const Matrix4 world = owner.GetWorldMatrix();
    const Vector3 pos{world.m[12], world.m[13], world.m[14]};
    if (!hasLastWorldPos || timing.deltaTimeSeconds <= 1.0e-6F) {
        lastWorldPos = pos;
        hasLastWorldPos = true;
        return 0.0F;
    }
    const float invDt = 1.0F / timing.deltaTimeSeconds;
    const float dx = pos.x - lastWorldPos.x;
    const float dz = pos.z - lastWorldPos.z;
    lastWorldPos = pos;
    return std::sqrt(dx * dx + dz * dz) * invDt;
}

void Character3DAnimFsmComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    AnimatorComponent* anim = owner.GetComponent<AnimatorComponent>();
    if (anim == nullptr || !anim->GetSkeleton()) {
        return;
    }

    const bool attackOk = ClipValid_(*anim, attackClip);
    const std::uint32_t cur = anim->GetClipIndex();
    const bool holdingAttack = attackOk && cur == attackClip && !anim->IsClipFinished();

    std::uint32_t want = idleClip;
    AnimLoopMode wantLoop = AnimLoopMode::Loop;

    if (holdingAttack) {
        want = attackClip;
        wantLoop = AnimLoopMode::Once;
    } else if (pendingAttack && attackOk) {
        want = attackClip;
        wantLoop = AnimLoopMode::Once;
    } else if (manualClipActive && ClipValid_(*anim, manualClip)) {
        want = manualClip;
        wantLoop = manualLoop;
    } else if (!locomotionDrivingEnabled) {
        pendingAttack = false;
        return;
    } else {
        const bool runOk = ClipValid_(*anim, runClip);
        const bool walkOk = ClipValid_(*anim, walkClip);

        bool moving = false;
        bool wantSprint = false;
        if (hasLocomotionInput) {
            moving = inputMoving;
            wantSprint = inputSprint;
        } else {
            const float speed = MeasureLocomotionSpeed_(owner, timing);
            moving = speed >= walkThresh;
            wantSprint = runOk && speed >= runThresh;
        }
        hasLocomotionInput = false;

        if (!moving) {
            if (ClipValid_(*anim, idleClip)) {
                want = idleClip;
            } else if (walkOk) {
                want = walkClip;
            }
        } else if (wantSprint && runOk) {
            want = runClip;
        } else if (walkOk) {
            want = walkClip;
        } else if (ClipValid_(*anim, idleClip)) {
            want = idleClip;
        }
    }
    pendingAttack = false;

    if (!ClipValid_(*anim, want) && ClipValid_(*anim, idleClip)) {
        want = idleClip;
        wantLoop = AnimLoopMode::Loop;
    }

    ApplyClip_(*anim, want, wantLoop);
}

}  // namespace Spark
