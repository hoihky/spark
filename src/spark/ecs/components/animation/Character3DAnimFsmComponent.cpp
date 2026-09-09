#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"

#include "spark/animation/AnimBlend1D.hpp"
#include "spark/animation/AnimLoopMode.hpp"
#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"

#include <cmath>

namespace Spark {

namespace {

constexpr int kCombatCmdHurt = 1;
constexpr int kCombatCmdAttack = 2;
constexpr int kCombatCmdStagger = 3;
constexpr int kCombatCmdDeath = 4;

[[nodiscard]] float HorizontalSpeedXZ(const Vector3& velocity) noexcept {
    return std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
}

[[nodiscard]] float MeasureLocomotionSpeedInHierarchy(
        const GameObject& owner,
        const FrameTiming& timing,
        Vector3& lastWorldPos,
        bool& hasLastWorldPos) noexcept {
    for (const GameObject* node = &owner; node != nullptr; node = node->GetParent()) {
        if (const Rigidbody3DComponent* rb = node->GetComponent<Rigidbody3DComponent>()) {
            return HorizontalSpeedXZ(rb->GetVelocity());
        }
        if (const CharacterController3DComponent* controller =
                    node->GetComponent<CharacterController3DComponent>()) {
            const Vector3& moveInput = controller->GetMoveInput();
            const float inputSpeed = HorizontalSpeedXZ(moveInput);
            if (inputSpeed > 1.0e-4F) {
                return inputSpeed;
            }
            return HorizontalSpeedXZ(controller->GetVelocity());
        }
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

}  // namespace

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
    SetCombatClips(clips.attack, clips.hurt, clips.stagger, clips.death);
}

void Character3DAnimFsmComponent::SetCombatClips(
        const std::uint32_t attackClipIndex,
        const std::uint32_t hurtClipIndex,
        const std::uint32_t staggerClipIndex,
        const std::uint32_t deathClipIndex) noexcept {
    attackClip = attackClipIndex;
    hurtClip = hurtClipIndex;
    staggerClip = staggerClipIndex;
    deathClip = deathClipIndex;
}

void Character3DAnimFsmComponent::SetAttackClip(const std::uint32_t attackClipIndex) noexcept {
    attackClip = attackClipIndex;
}

void Character3DAnimFsmComponent::SetHurtClip(const std::uint32_t hurtClipIndex) noexcept {
    hurtClip = hurtClipIndex;
}

void Character3DAnimFsmComponent::SetStaggerClip(const std::uint32_t staggerClipIndex) noexcept {
    staggerClip = staggerClipIndex;
}

void Character3DAnimFsmComponent::SetDeathClip(const std::uint32_t deathClipIndex) noexcept {
    deathClip = deathClipIndex;
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

void Character3DAnimFsmComponent::RequestHurt() noexcept {
    pendingCombat = static_cast<std::uint8_t>(kCombatCmdHurt);
}

void Character3DAnimFsmComponent::RequestAttack() noexcept {
    pendingCombat = static_cast<std::uint8_t>(kCombatCmdAttack);
}

void Character3DAnimFsmComponent::RequestStagger() noexcept {
    pendingCombat = static_cast<std::uint8_t>(kCombatCmdStagger);
}

void Character3DAnimFsmComponent::RequestDeath() noexcept {
    pendingCombat = static_cast<std::uint8_t>(kCombatCmdDeath);
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

void Character3DAnimFsmComponent::SetLocomotionAnalogSpeed(const float speedMetersPerSecond) noexcept {
    hasLocomotionAnalog = true;
    locomotionAnalogSpeed = (speedMetersPerSecond > 0.0F) ? speedMetersPerSecond : 0.0F;
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
    anim.ClearLocomotionBlend();

    const bool sameClip = want == anim.GetClipIndex();
    const AnimLoopMode prevLoop = anim.GetLoopMode();
    if (sameClip && prevLoop == loopMode && !anim.IsCrossfading() && !anim.IsLocomotionBlending()
            && !(loopMode == AnimLoopMode::Once && anim.IsClipFinished())) {
        return;
    }

    anim.SetLoopMode(loopMode);
    if (!sameClip) {
        anim.SetClipIndexWithCrossfade(want, crossfadeDuration);
        return;
    }

    // Same clip, loop mode changed (e.g. combat Once -> locomotion Loop on shared Run clip).
    if (loopMode == AnimLoopMode::Loop && prevLoop != AnimLoopMode::Loop) {
        if (const SharedPtr<Skeleton>& sk = anim.GetSkeleton()) {
            const float dur = sk->GetClipDuration(want);
            if (dur > 1.0e-4F && anim.GetTimeSeconds() >= dur - 1.0e-4F) {
                anim.SetTimeSeconds(0.0F);
            }
        }
    } else if (loopMode == AnimLoopMode::Once) {
        anim.RestartCurrentClip();
        anim.SetLoopMode(AnimLoopMode::Once);
    }
}

void Character3DAnimFsmComponent::ApplyLocomotionBlend_(
        AnimatorComponent& anim,
        const float speedMetersPerSecond) noexcept {
    AnimBlend1DKeyframe keyframes[3]{};
    std::size_t keyframeCount = 0;

    const bool idleOk = ClipValid_(anim, idleClip);
    const bool walkOk = ClipValid_(anim, walkClip);
    const bool runOk = ClipValid_(anim, runClip);

    if (idleOk) {
        keyframes[keyframeCount++] = {0.0F, idleClip};
    }
    if (walkOk) {
        keyframes[keyframeCount++] = {walkThresh, walkClip};
    }
    if (runOk) {
        keyframes[keyframeCount++] = {runThresh, runClip};
    }

    if (keyframeCount == 0) {
        return;
    }

    const AnimBlend1DSample sample = EvaluateAnimBlend1D(keyframes, keyframeCount, speedMetersPerSecond);
    if (!sample.valid) {
        return;
    }

    anim.SetLoopMode(AnimLoopMode::Loop);
    if (sample.clipA == sample.clipB) {
        ApplyClip_(anim, sample.clipA, AnimLoopMode::Loop);
        return;
    }

    anim.SetLocomotionBlend(sample.clipA, sample.clipB, sample.blend01);
}

void Character3DAnimFsmComponent::ApplyLocomotionDiscrete_(
        AnimatorComponent& anim,
        const bool moving,
        const bool wantSprint) noexcept {
    const bool runOk = ClipValid_(anim, runClip);
    const bool walkOk = ClipValid_(anim, walkClip);

    std::uint32_t want = idleClip;
    if (!moving) {
        if (ClipValid_(anim, idleClip)) {
            want = idleClip;
        } else if (walkOk) {
            want = walkClip;
        }
    } else if (wantSprint && runOk) {
        want = runClip;
    } else if (walkOk) {
        want = walkClip;
    } else if (ClipValid_(anim, idleClip)) {
        want = idleClip;
    }

    if (!ClipValid_(anim, want) && ClipValid_(anim, idleClip)) {
        want = idleClip;
    }

    ApplyClip_(anim, want, AnimLoopMode::Loop);
}

void Character3DAnimFsmComponent::MaybeClearBlackboardCombat_(GameObject& owner, const int playingCmd) noexcept {
    if (combatBbSlot == static_cast<std::size_t>(-1) || playingCmd == 0) {
        return;
    }
    AiAgentComponent* ai = owner.GetComponent<AiAgentComponent>();
    if (ai == nullptr || !ai->IsEnabled()) {
        return;
    }
    if (ai->GetBlackboard().GetInt(combatBbSlot) == playingCmd) {
        ai->GetBlackboard().SetInt(combatBbSlot, 0);
    }
}

Character3DAnimFsmComponent::CombatSelection Character3DAnimFsmComponent::ResolveCombat_(
        AnimatorComponent& anim,
        GameObject& owner,
        const int combatCmd) noexcept {
    CombatSelection out{};

    const bool hurtOk = ClipValid_(anim, hurtClip);
    const bool attackOk = ClipValid_(anim, attackClip);
    const bool staggerOk = ClipValid_(anim, staggerClip);
    const bool deathOk = ClipValid_(anim, deathClip);
    const std::uint32_t cur = anim.GetClipIndex();

    if (deathLocked) {
        if (deathOk) {
            out.clip = deathClip;
            out.loopMode = AnimLoopMode::Hold;
            out.active = true;
            out.playingCmd = kCombatCmdDeath;
        }
        return out;
    }

    if (deathOk && cur == deathClip && !anim.IsClipFinished()) {
        out.clip = deathClip;
        out.loopMode = AnimLoopMode::Once;
        out.active = true;
        out.playingCmd = kCombatCmdDeath;
        return out;
    }
    if (deathOk && cur == deathClip && anim.IsClipFinished()) {
        deathLocked = true;
        MaybeClearBlackboardCombat_(owner, kCombatCmdDeath);
        out.clip = deathClip;
        out.loopMode = AnimLoopMode::Hold;
        out.active = true;
        out.playingCmd = kCombatCmdDeath;
        return out;
    }

    if (hurtOk && cur == hurtClip && !anim.IsClipFinished()
            && anim.GetLoopMode() == AnimLoopMode::Once) {
        MaybeClearBlackboardCombat_(owner, kCombatCmdHurt);
        out.clip = hurtClip;
        out.loopMode = AnimLoopMode::Once;
        out.active = true;
        out.playingCmd = kCombatCmdHurt;
        return out;
    }
    if (hurtOk && cur == hurtClip && anim.IsClipFinished()) {
        MaybeClearBlackboardCombat_(owner, kCombatCmdHurt);
    }

    if (staggerOk && cur == staggerClip && !anim.IsClipFinished()
            && anim.GetLoopMode() == AnimLoopMode::Once) {
        MaybeClearBlackboardCombat_(owner, kCombatCmdStagger);
        out.clip = staggerClip;
        out.loopMode = AnimLoopMode::Once;
        out.active = true;
        out.playingCmd = kCombatCmdStagger;
        return out;
    }
    if (staggerOk && cur == staggerClip && anim.IsClipFinished()) {
        MaybeClearBlackboardCombat_(owner, kCombatCmdStagger);
    }

    // Locomotion clips may share the same index as attack (e.g. Fox Run). Only treat Once playback as combat.
    const bool holdingAttack = attackOk && cur == attackClip && !anim.IsClipFinished()
            && anim.GetLoopMode() == AnimLoopMode::Once;
    if (holdingAttack) {
        if (combatCmd == kCombatCmdDeath && deathOk) {
            out.clip = deathClip;
            out.loopMode = AnimLoopMode::Once;
            out.active = true;
            out.playingCmd = kCombatCmdDeath;
            return out;
        }
        if (combatCmd == kCombatCmdHurt && hurtOk) {
            out.clip = hurtClip;
            out.loopMode = AnimLoopMode::Once;
            out.active = true;
            out.playingCmd = kCombatCmdHurt;
            return out;
        }
        if (combatCmd == kCombatCmdStagger && staggerOk) {
            out.clip = staggerClip;
            out.loopMode = AnimLoopMode::Once;
            out.active = true;
            out.playingCmd = kCombatCmdStagger;
            return out;
        }
        out.clip = attackClip;
        out.loopMode = AnimLoopMode::Once;
        out.active = true;
        out.playingCmd = kCombatCmdAttack;
        return out;
    }
    if (attackOk && cur == attackClip && anim.IsClipFinished()) {
        MaybeClearBlackboardCombat_(owner, kCombatCmdAttack);
    }

    if (combatCmd == kCombatCmdDeath && deathOk) {
        out.clip = deathClip;
        out.loopMode = AnimLoopMode::Once;
        out.active = true;
        out.playingCmd = kCombatCmdDeath;
    } else if (combatCmd == kCombatCmdHurt && hurtOk) {
        out.clip = hurtClip;
        out.loopMode = AnimLoopMode::Once;
        out.active = true;
        out.playingCmd = kCombatCmdHurt;
    } else if (combatCmd == kCombatCmdStagger && staggerOk) {
        out.clip = staggerClip;
        out.loopMode = AnimLoopMode::Once;
        out.active = true;
        out.playingCmd = kCombatCmdStagger;
    } else if (combatCmd == kCombatCmdAttack && attackOk) {
        out.clip = attackClip;
        out.loopMode = AnimLoopMode::Once;
        out.active = true;
        out.playingCmd = kCombatCmdAttack;
    }

    return out;
}

float Character3DAnimFsmComponent::MeasureLocomotionSpeed_(
        const GameObject& owner,
        const FrameTiming& timing) noexcept {
    return MeasureLocomotionSpeedInHierarchy(owner, timing, lastWorldPos, hasLastWorldPos);
}

float Character3DAnimFsmComponent::ResolveLocomotionSpeedParam_(
        const GameObject& owner,
        const FrameTiming& timing,
        const bool moving,
        const bool wantSprint) noexcept {
    if (hasLocomotionInput) {
        if (!moving) {
            return 0.0F;
        }
        if (wantSprint) {
            return runThresh;
        }
        return walkThresh;
    }
    return MeasureLocomotionSpeed_(owner, timing);
}

void Character3DAnimFsmComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    AnimatorComponent* anim = owner.GetComponent<AnimatorComponent>();
    if (anim == nullptr || !anim->GetSkeleton()) {
        return;
    }

    int combatCmd = static_cast<int>(pendingCombat);
    pendingCombat = 0;

    if (combatBbSlot != static_cast<std::size_t>(-1)) {
        if (AiAgentComponent* ai = owner.GetComponent<AiAgentComponent>()) {
            if (ai->IsEnabled()) {
                const int bbCmd = ai->GetBlackboard().GetInt(combatBbSlot);
                if (bbCmd > combatCmd) {
                    combatCmd = bbCmd;
                }
            }
        }
    }

    if (deathLocked) {
        const CombatSelection combat = ResolveCombat_(*anim, owner, combatCmd);
        if (combat.active) {
            ApplyClip_(*anim, combat.clip, combat.loopMode);
        }
        return;
    }

    const CombatSelection combat = ResolveCombat_(*anim, owner, combatCmd);
    if (combat.active) {
        ApplyClip_(*anim, combat.clip, combat.loopMode);
        return;
    }

    if (manualClipActive && ClipValid_(*anim, manualClip)) {
        ApplyClip_(*anim, manualClip, manualLoop);
        return;
    }

    if (!locomotionDrivingEnabled) {
        return;
    }

    const bool runOk = ClipValid_(*anim, runClip);
    if (hasLocomotionAnalog) {
        ApplyLocomotionBlend_(*anim, locomotionAnalogSpeed);
        hasLocomotionAnalog = false;
        return;
    }

    bool moving = false;
    bool wantSprint = false;
    const bool usedLocomotionInput = hasLocomotionInput;
    if (hasLocomotionInput) {
        moving = inputMoving;
        wantSprint = inputSprint && runOk;
    } else {
        const float speed = MeasureLocomotionSpeed_(owner, timing);
        moving = speed >= walkThresh;
        wantSprint = runOk && speed >= runThresh;
    }
    hasLocomotionInput = false;

    if (locomotionBlendEnabled && !usedLocomotionInput) {
        const float speedParam = ResolveLocomotionSpeedParam_(owner, timing, moving, wantSprint);
        ApplyLocomotionBlend_(*anim, speedParam);
        return;
    }

    ApplyLocomotionDiscrete_(*anim, moving, wantSprint);
}

}  // namespace Spark
