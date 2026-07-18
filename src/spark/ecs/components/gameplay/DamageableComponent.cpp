#include "spark/ecs/components/gameplay/DamageableComponent.hpp"

#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

float DamageableComponent::ApplyDamage(const float amount, GameObject* instigator) {
    if (invulnerable || amount <= 0.0F) {
        return 0.0F;
    }
    GameObject* owner = GetOwner();
    if (owner == nullptr) {
        return 0.0F;
    }
    HealthComponent* health = owner->GetComponent<HealthComponent>();
    if (health == nullptr) {
        return 0.0F;
    }
    return health->ApplyDamage(amount * damageMultiplier, instigator);
}

}  // namespace Spark
