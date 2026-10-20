#include "spark/ecs/components/animation/AnimationHitbox2DComponent.hpp"

#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/gameplay/Combat2D.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

bool ArrayContainsObject(const Array<GameObject*>& list, const GameObject* object) noexcept {
    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        if (list[i] == object) {
            return true;
        }
    }
    return false;
}

}  // namespace

void AnimationHitbox2DComponent::SetRadius(const float value) noexcept {
    radius = std::max(0.05F, value);
}

void AnimationHitbox2DComponent::SetArcHalfAngleRadians(const float radians) noexcept {
    arcHalfAngleRadians = std::clamp(radians, 0.05F, 3.1F);
}

void AnimationHitbox2DComponent::AddTarget(GameObject* target) noexcept {
    if (target == nullptr || ArrayContainsObject(targets, target)) {
        return;
    }
    targets.PushBack(target);
}

void AnimationHitbox2DComponent::ResetSwingHits() noexcept {
    hitThisSwing.Clear();
    hitsThisSwing = 0;
}

bool AnimationHitbox2DComponent::IsFrameInWindow(const SpriteAnimatorComponent& animator) const noexcept {
    if (animator.GetClipIndex() != clipIndex) {
        return false;
    }
    const std::uint32_t frame = animator.GetCurrentLocalFrame();
    return frame >= startLocalFrame && frame <= endLocalFrame;
}

float AnimationHitbox2DComponent::ResolveFacingSign(const GameObject& owner) const noexcept {
    const TransformComponent* transform = owner.GetComponent<TransformComponent>();
    if (transform == nullptr) {
        return 1.0F;
    }
    const Vector3 scale = transform->GetLocalTransform().scale;
    return (scale.x < 0.0F) ? -1.0F : 1.0F;
}

void AnimationHitbox2DComponent::ResolveWorldOrigin(
        const GameObject& owner,
        float& outX,
        float& outY) const noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    const Vector3 local{localOffset.x, localOffset.y, 0.0F};
    const Vector3 worldPos = world.TranslationVector() + world.TransformVector(local);
    outX = worldPos.x;
    outY = worldPos.y;
}

void AnimationHitbox2DComponent::TryOverlapHits(GameObject& owner, GameWorld& world) noexcept {
    float originX = 0.0F;
    float originY = 0.0F;
    ResolveWorldOrigin(owner, originX, originY);
    const float facing = ResolveFacingSign(owner);
    const float dirX = facing;
    const float dirY = 0.0F;

    auto tryDamageTarget = [&](GameObject* target) {
        if (target == nullptr || target == &owner || !target->IsActiveInHierarchy()) {
            return;
        }
        if (ArrayContainsObject(hitThisSwing, target)) {
            return;
        }
        const float applied = Combat2DTarget{*target}.ApplyDamage(&owner, damagePerHit);
        if (applied > 0.0F) {
            hitThisSwing.PushBack(target);
            ++hitsThisSwing;
        }
    };

    for (std::size_t i = 0; i < targets.GetSize(); ++i) {
        tryDamageTarget(targets[i]);
    }

    CombatHurtboxWorldQuery2D hurtboxQuery{};
    Array<CombatHurtboxOverlap2D> overlaps{};
    if (shape == AnimationHitbox2DShape::Arc) {
        hurtboxQuery.CollectArcOverlaps(
                world,
                originX,
                originY,
                radius,
                dirX,
                dirY,
                arcHalfAngleRadians,
                queryFilter,
                &owner,
                overlaps);
    } else {
        hurtboxQuery.CollectCircleOverlaps(
                world, originX, originY, radius, queryFilter, &owner, overlaps);
    }
    for (std::size_t i = 0; i < overlaps.GetSize(); ++i) {
        tryDamageTarget(overlaps[i].GetOwner());
    }
}

void AnimationHitbox2DComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& /*context*/) {
    const SpriteAnimatorComponent* animator = owner.GetComponent<SpriteAnimatorComponent>();
    if (animator == nullptr) {
        hitWindowActive = false;
        return;
    }

    const bool inWindow = IsFrameInWindow(*animator);
    if (inWindow && !hitWindowActive) {
        ResetSwingHits();
        TryOverlapHits(owner, owner.GetWorld());
    }
    hitWindowActive = inWindow;
}

}  // namespace Spark
