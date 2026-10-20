#include "spark/animation/combat/Character3DCombatGraphDriver.hpp"

#include "spark/animation/combat/CombatActionSpec.hpp"
#include "spark/animation/AnimLoopMode.hpp"
#include "spark/ecs/components/animation/AnimationLayerStackComponent.hpp"
#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/character/Character3DBodyComponent.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/animation/Skeleton.hpp"

namespace Spark {

namespace {

CombatActionKind ToActionKind(const int value) noexcept {
    return CombatActionKind::FromInt(value);
}

}  // namespace

bool Character3DCombatGraphDriver::ClipValid_(const AnimatorComponent& anim, const std::uint32_t clip) const noexcept {
    return clip < anim.GetClipCount();
}

void Character3DCombatGraphDriver::RebuildActionSpecs() noexcept {
    graph.Reset();

    auto registerOne = [&](CombatActionKind::Value kind, std::uint32_t clip, int priority, bool rootMotion) {
        CombatActionSpec spec{};
        spec.SetKind(CombatActionKind{kind});
        spec.SetClipIndex(clip);
        spec.SetUpperBodyOverlay(owner.GetUseUpperBodyCombatOverlay());
        spec.SetUseAttackRootMotion(rootMotion);
        spec.SetInterruptPriority(priority);
        graph.RegisterAction(spec);
    };

    registerOne(CombatActionKind::Value::Attack, owner.GetAttackClipIndex(), 1, true);
    registerOne(CombatActionKind::Value::Hurt, owner.GetHurtClipIndex(), 2, false);
    registerOne(CombatActionKind::Value::Stagger, owner.GetStaggerClipIndex(), 3, false);
    registerOne(CombatActionKind::Value::Death, owner.GetDeathClipIndex(), 4, false);
}

void Character3DCombatGraphDriver::EndRootMotion_(GameObject& visual) noexcept {
    if (Character3DBodyComponent* body = Character3DBodyComponent::FindOnAncestors(visual)) {
        body->EndAttackRootMotionWindow();
    }
}

void Character3DCombatGraphDriver::StartOverlay_(
        AnimationLayerStackComponent& stack,
        AnimatorComponent& anim,
        GameObject& visual,
        const CombatActionSpec& spec) noexcept {
    if (!ClipValid_(anim, spec.GetClipIndex())) {
        return;
    }
    stack.PlayOverlay(spec.GetClipIndex(), AnimLoopMode::Once);
    presentingAction = spec.GetKind();
    if (spec.UsesAttackRootMotion()) {
        if (Character3DBodyComponent* body = Character3DBodyComponent::FindOnAncestors(visual)) {
            body->BeginAttackRootMotionWindow();
        }
    } else {
        EndRootMotion_(visual);
    }
    owner.NotifyCombatBlackboardCleared(visual, spec.GetKind());
}

void Character3DCombatGraphDriver::ProcessFrame(
        AnimatorComponent& anim,
        GameObject& visual,
        CombatActionKind& pendingAction) noexcept {
    if (!owner.GetUseUpperBodyCombatOverlay()) {
        return;
    }
    AnimationLayerStackComponent* stack = visual.GetComponent<AnimationLayerStackComponent>();
    if (stack == nullptr) {
        return;
    }

    if (pendingAction.GetValue() == CombatActionKind::Value::Death) {
        if (stack->IsOverlayActive()) {
            stack->ClearOverlay();
        }
        presentingAction = CombatActionKind{};
        EndRootMotion_(visual);
        pendingAction = CombatActionKind{};
        return;
    }

    if (pendingAction.GetValue() != CombatActionKind::Value::None) {
        graph.Request(pendingAction);
        pendingAction = CombatActionKind{};
    }

    if (stack->IsOverlayActive() && stack->IsOverlayFinished()) {
        stack->ClearOverlay();
        EndRootMotion_(visual);
        if (presentingAction.GetValue() != CombatActionKind::Value::None) {
            owner.NotifyCombatBlackboardCleared(visual, presentingAction);
        }
        presentingAction = CombatActionKind{};
        graph.AdvanceOverlayPlayback(1.0F, true);
    }

    const bool overlayActive = stack->IsOverlayActive() && !stack->IsOverlayFinished();
    if (overlayActive) {
        float normalized = 0.0F;
        if (const SharedPtr<Skeleton>& sk = anim.GetSkeleton()) {
            const float dur = sk->GetClipDuration(stack->GetOverlayClipIndex());
            if (dur > 1.0e-4F) {
                normalized = stack->GetOverlayTimeSeconds() / dur;
            }
        }
        graph.AdvanceOverlayPlayback(normalized, false);
        pendingAction = CombatActionKind{};
        return;
    }

    if (graph.HasActiveAction()) {
        const CombatActionSpec* spec = graph.FindSpec(graph.GetActiveAction());
        if (spec != nullptr && spec->UsesUpperBodyOverlay()) {
            StartOverlay_(*stack, anim, visual, *spec);
        }
        graph.AdvanceOverlayPlayback(0.0F, false);
    }
}

bool Character3DCombatGraphDriver::IsOverlayPresenting() const noexcept {
    return presentingAction.GetValue() != CombatActionKind::Value::None
            || graph.HasActiveAction();
}

}  // namespace Spark
