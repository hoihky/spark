#include "spark/ecs/components/animation/Sprite2DCharacterAnimFsmComponent.hpp"

#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/math/Vector2.hpp"

#include <cmath>

namespace Spark {

void Sprite2DCharacterAnimFsmComponent::SetLocomotionClips(
        const std::uint32_t idleClipIndex, const std::uint32_t moveClipIndex) noexcept {
    idleClip = idleClipIndex;
    moveClip = moveClipIndex;
}

void Sprite2DCharacterAnimFsmComponent::SetCombatClips(
        const std::uint32_t attackClipIndex, const std::uint32_t hurtClipIndex) noexcept {
    attackClip = attackClipIndex;
    hurtClip = hurtClipIndex;
}

void Sprite2DCharacterAnimFsmComponent::SetMoveSpeedThreshold(const float worldUnits) noexcept {
    moveThresh = (worldUnits > 1.0e-6F) ? worldUnits : 0.12F;
}

void Sprite2DCharacterAnimFsmComponent::RequestHurt() noexcept {
    pendingCombat = 1;
}

void Sprite2DCharacterAnimFsmComponent::RequestAttack() noexcept {
    pendingCombat = 2;
}

bool Sprite2DCharacterAnimFsmComponent::ClipValid_(const SpriteAnimatorComponent& anim, const std::uint32_t clip)
        const noexcept {
    return clip < anim.GetClipCount();
}

void Sprite2DCharacterAnimFsmComponent::MaybeClearBlackboardCombat_(GameObject& owner, const int playingCmd) noexcept {
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

void Sprite2DCharacterAnimFsmComponent::OnUpdate(
        const FrameTiming& /*timing*/, GameObject& owner, IEngineContext& /*context*/) {
    SpriteAnimatorComponent* anim = owner.GetComponent<SpriteAnimatorComponent>();
    if (anim == nullptr) {
        return;
    }

    int cmd = static_cast<int>(pendingCombat);
    pendingCombat = 0;

    if (combatBbSlot != static_cast<std::size_t>(-1)) {
        if (AiAgentComponent* ai = owner.GetComponent<AiAgentComponent>()) {
            if (ai->IsEnabled()) {
                const int bbCmd = ai->GetBlackboard().GetInt(combatBbSlot);
                if (bbCmd > cmd) {
                    cmd = bbCmd;
                }
            }
        }
    }

    const bool hurtOk = ClipValid_(*anim, hurtClip);
    const bool attackOk = ClipValid_(*anim, attackClip);
    const std::uint32_t cur = anim->GetClipIndex();

    if (hurtOk && cur == hurtClip && anim->IsCurrentClipFinished()) {
        MaybeClearBlackboardCombat_(owner, 1);
    }
    if (attackOk && cur == attackClip && anim->IsCurrentClipFinished()) {
        MaybeClearBlackboardCombat_(owner, 2);
    }

    std::uint32_t want = idleClip;

    const bool holdingHurt = hurtOk && cur == hurtClip && !anim->IsCurrentClipFinished();
    const bool holdingAttack = attackOk && cur == attackClip && !anim->IsCurrentClipFinished();

    if (holdingHurt) {
        want = hurtClip;
    } else if (holdingAttack) {
        if (cmd == 1 && hurtOk) {
            want = hurtClip;
        } else {
            want = attackClip;
        }
    } else if (cmd == 1 && hurtOk) {
        want = hurtClip;
    } else if (cmd == 2 && attackOk) {
        want = attackClip;
    } else {
        if (Rigidbody2DComponent* rb = owner.GetComponent<Rigidbody2DComponent>()) {
            const Vector2 v = rb->GetVelocity();
            float measure = 0.0F;
            if (locomotionSource == Sprite2DAnimLocomotionSource::HorizontalAbsVelX) {
                measure = std::fabs(v.x);
            } else {
                measure = std::sqrt(v.x * v.x + v.y * v.y);
            }
            if (measure > moveThresh && ClipValid_(*anim, moveClip)) {
                want = moveClip;
            } else if (ClipValid_(*anim, idleClip)) {
                want = idleClip;
            }
        } else if (ClipValid_(*anim, idleClip)) {
            want = idleClip;
        }
    }

    if (!ClipValid_(*anim, want) && ClipValid_(*anim, idleClip)) {
        want = idleClip;
    }

    anim->SetClipIndex(want);
}

}  // namespace Spark
