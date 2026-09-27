#include "spark/gameplay/CombatDamage2D.hpp"

#include "spark/ecs/components/gameplay/DamageableComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/physics/2d/Hurtbox2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

float TryApplyCombatDamage2D(GameObject& target, GameObject* instigator, const float rawDamage) noexcept {
    if (rawDamage <= 0.0F || !target.IsActiveInHierarchy()) {
        return 0.0F;
    }

    Hurtbox2DComponent* hurtbox = target.GetComponent<Hurtbox2DComponent>();
    if (hurtbox != nullptr) {
        if (!hurtbox->IsEnabled() || hurtbox->IsInvulnerable()) {
            return 0.0F;
        }
    }

    GameObject* damageReceiver = &target;
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

}  // namespace Spark
