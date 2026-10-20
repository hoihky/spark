#include "spark/ecs/components/animation/AnimationMeleeHitComponent.hpp"

#include "spark/animation/AnimLoopMode.hpp"
#include "spark/ecs/components/animation/AnimationLayerStackComponent.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/gameplay/Combat3D.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/mesh/MeshRaycast.hpp"

#include <cmath>
#include <cstring>

namespace Spark {

namespace {

bool EventNameEquals(const void* ptr, const char* expected) noexcept {
    if (ptr == nullptr || expected == nullptr) {
        return false;
    }
    return std::strcmp(static_cast<const char*>(ptr), expected) == 0;
}

bool ArrayContainsObject(const Array<GameObject*>& list, const GameObject* object) noexcept {
    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        if (list[i] == object) {
            return true;
        }
    }
    return false;
}

}  // namespace

void AnimationMeleeHitComponent::SetStartEventName(const char* name) noexcept {
    startEventName = Utf8String(name != nullptr ? name : "active_start");
}

void AnimationMeleeHitComponent::SetEndEventName(const char* name) noexcept {
    endEventName = Utf8String(name != nullptr ? name : "active_end");
}

void AnimationMeleeHitComponent::SetTraceDistance(const float meters) noexcept {
    traceDistance = (meters > 1.0e-3F) ? meters : 2.2F;
}

void AnimationMeleeHitComponent::SetHitRadius(const float meters) noexcept {
    hitRadius = (meters > 1.0e-3F) ? meters : 0.65F;
}

void AnimationMeleeHitComponent::AddTarget(GameObject* target) noexcept {
    if (target == nullptr || ArrayContainsObject(targets, target)) {
        return;
    }
    targets.PushBack(target);
}

void AnimationMeleeHitComponent::ResetSwingHits() noexcept {
    hitThisSwing.Clear();
    hitsThisSwing = 0;
}

void AnimationMeleeHitComponent::OnSignal(
        GameObject& /*owner*/,
        const SignalId id,
        const SignalPayload& payload) {
    if (id != SignalId::AnimationEvent) {
        return;
    }
    if (EventNameEquals(payload.ptr, startEventName.CStr())) {
        hitWindowActive = true;
        ResetSwingHits();
        return;
    }
    if (EventNameEquals(payload.ptr, endEventName.CStr())) {
        hitWindowActive = false;
    }
}

Vector3 AnimationMeleeHitComponent::ResolveForward_(const GameObject& owner) const noexcept {
    const GameObject* basis = facingObject != nullptr ? facingObject : &owner;
    const Matrix4 world = basis->GetWorldMatrix();
    Vector3 forward{-world.m[8], 0.0F, -world.m[10]};
    const float len2 = forward.LengthSquared();
    if (len2 <= 1.0e-8F) {
        return Vector3{0.0F, 0.0F, -1.0F};
    }
    return forward * (1.0F / std::sqrt(len2));
}

Vector3 AnimationMeleeHitComponent::ResolveOrigin_(const GameObject& owner) const noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    return world.TranslationVector() + world.TransformVector(originLocalOffset);
}

void AnimationMeleeHitComponent::RegisterHit_(GameObject& owner, GameObject& target) noexcept {
    if (ArrayContainsObject(hitThisSwing, &target)) {
        return;
    }
    Combat3DTarget combatTarget{target};
    const float applied = combatTarget.ApplyDamage(&owner, damagePerHit);
    if (applied <= 0.0F) {
        return;
    }
    if (applyHurtReaction) {
        combatTarget.RequestHurtReaction();
    }
    if (knockbackMeters > 0.0F) {
        combatTarget.ApplyKnockback(ResolveForward_(owner), knockbackMeters);
    }
    hitThisSwing.PushBack(&target);
    ++hitsThisSwing;
    ++totalHits;
}

void AnimationMeleeHitComponent::TryQueryWorldHurtboxes_(
        GameObject& owner,
        const Vector3& sampleCenter) noexcept {
    CombatHurtboxSphereQuery hurtboxQuery{owner.GetWorld()};
    hurtboxQuery.SetFilter(queryFilter);
    Array<CombatHurtboxOverlap> hits{};
    hurtboxQuery.CollectOverlaps(sampleCenter, hitRadius, hits);
    for (std::size_t i = 0; i < hits.GetSize(); ++i) {
        if (hits[i].GetOwner() == nullptr) {
            continue;
        }
        RegisterHit_(owner, *hits[i].GetOwner());
    }
}

void AnimationMeleeHitComponent::TryTraceHits(GameObject& owner) noexcept {
    const Vector3 origin = ResolveOrigin_(owner);
    const Vector3 forward = ResolveForward_(owner);
    const Vector3 mid = origin + forward * (traceDistance * 0.5F);
    const Vector3 tip = origin + forward * traceDistance;

    if (worldHurtboxQueryEnabled) {
        TryQueryWorldHurtboxes_(owner, origin);
        TryQueryWorldHurtboxes_(owner, mid);
        TryQueryWorldHurtboxes_(owner, tip);
    }

    for (std::size_t i = 0; i < targets.GetSize(); ++i) {
        GameObject* target = targets[i];
        if (target == nullptr || !target->IsActiveInHierarchy()) {
            continue;
        }
        if (ArrayContainsObject(hitThisSwing, target)) {
            continue;
        }
        const Vector3 center = target->GetWorldMatrix().TranslationVector() + Vector3{0.0F, 0.9F, 0.0F};
        const float targetRadius = 0.55F;

        float bestT = traceDistance + 1.0F;
        float hitT = 0.0F;
        bool hit = false;
        if (TryRaycastSphereWorld(origin, forward, center, targetRadius, 1.0e-4F, bestT, hitT) && hitT > 0.0F) {
            hit = true;
        }
        if (!hit) {
            bestT = traceDistance + 1.0F;
            if (TryRaycastSphereWorld(mid, forward, center, targetRadius, 1.0e-4F, bestT, hitT) && hitT > 0.0F) {
                hit = true;
            }
        }
        if (!hit) {
            const Vector3 toCenter = center - tip;
            if (toCenter.LengthSquared() <= (hitRadius + targetRadius) * (hitRadius + targetRadius)) {
                hit = true;
            }
        }
        if (!hit) {
            continue;
        }
        RegisterHit_(owner, *target);
    }
}

void AnimationMeleeHitComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (const AnimationLayerStackComponent* stack = owner.GetComponent<AnimationLayerStackComponent>()) {
        if (stack->IsOverlayActive() && !stack->IsOverlayFinished()) {
            if (!overlayHitWindow) {
                overlayHitWindow = true;
                ResetSwingHits();
            }
        } else {
            overlayHitWindow = false;
        }
    } else {
        overlayHitWindow = false;
    }

    if (!hitWindowActive && !overlayHitWindow) {
        return;
    }
    if (const AnimatorComponent* animator = owner.GetComponent<AnimatorComponent>()) {
        if (animator->GetLoopMode() == AnimLoopMode::Once && animator->IsClipFinished()) {
            hitWindowActive = false;
            return;
        }
    }
    TryTraceHits(owner);
}

}  // namespace Spark
