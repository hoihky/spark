#include "spark/gameplay/Combat3D.hpp"

#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/character/Character3DBodyComponent.hpp"
#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/physics/3d/Hurtbox3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/core/ColliderFilter.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>

namespace Spark {

float Combat3DTarget::ApplyDamage(GameObject* instigator, const float rawDamage) noexcept {
    if (target == nullptr || rawDamage <= 0.0F || !target->IsActiveInHierarchy()) {
        return 0.0F;
    }

    Hurtbox3DComponent* hurtbox = target->GetComponent<Hurtbox3DComponent>();
    if (hurtbox != nullptr) {
        if (!hurtbox->IsEnabled() || hurtbox->IsInvulnerable()) {
            return 0.0F;
        }
    }

    GameObject* damageReceiver = target;
    while (damageReceiver != nullptr) {
        if (damageReceiver->GetComponent<DamageableComponent>() != nullptr ||
            damageReceiver->GetComponent<HealthComponent>() != nullptr) {
            break;
        }
        damageReceiver = damageReceiver->GetParent();
    }
    if (damageReceiver == nullptr) {
        return 0.0F;
    }

    float applied = 0.0F;
    if (DamageableComponent* damageable = damageReceiver->GetComponent<DamageableComponent>()) {
        applied = damageable->ApplyDamage(rawDamage, instigator);
    } else if (HealthComponent* health = damageReceiver->GetComponent<HealthComponent>()) {
        applied = health->ApplyDamage(rawDamage, instigator);
    }

    if (applied > 0.0F && hurtbox != nullptr) {
        hurtbox->NotifyDamageReceived();
    }
    return applied;
}

void Combat3DTarget::ApplyKnockback(const Vector3& worldDirection, const float meters) noexcept {
    if (target == nullptr || meters <= 1.0e-5F) {
        return;
    }
    Character3DBodyComponent* body = Character3DBodyComponent::FindOnAncestors(*target);
    if (body == nullptr) {
        return;
    }
    Vector3 dir = worldDirection;
    const float len2 = dir.LengthSquared();
    if (len2 <= 1.0e-8F) {
        return;
    }
    dir = dir * (1.0F / std::sqrt(len2));
    body->GetMotor().ApplyWorldDelta(dir * meters);
}

void Combat3DTarget::RequestHurtReaction() noexcept {
    if (target == nullptr) {
        return;
    }
    if (Character3DAnimFsmComponent* fsm = Character3DAnimFsmComponent::FindInHierarchy(*target)) {
        fsm->RequestHurt();
    }
}

void CombatHurtboxSphereQuery::CollectOverlaps(
        const Vector3& center,
        const float radius,
        Array<CombatHurtboxOverlap>& outHits) noexcept {
    outHits.Clear();
    if (world == nullptr || radius <= 0.0F) {
        return;
    }

    ColliderFilter queryFilter{};
    queryFilter.categoryBits = filter.queryCategoryBits;
    queryFilter.maskBits = filter.queryMaskBits;
    queryFilter.isTrigger = true;

    world->ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        Hurtbox3DComponent* hurtbox = object->GetComponent<Hurtbox3DComponent>();
        if (hurtbox == nullptr || !hurtbox->IsEnabled()) {
            return;
        }
        if (!hurtbox->MatchesQueryFilter(queryFilter)) {
            return;
        }
        if (!hurtbox->OverlapsWorldSphere(*object, center, radius)) {
            return;
        }
        outHits.PushBack(CombatHurtboxOverlap{object, hurtbox});
    });
}

}  // namespace Spark
