#pragma once

namespace Spark {

class GameObject;

/** Routes damage to <c>DamageableComponent</c> / <c>HealthComponent</c> on the hierarchy. */
class GameplayDamageApplier final {
public:
    class IHurtboxNotifier {
    public:
        virtual ~IHurtboxNotifier() = default;
        virtual void NotifyDamageReceived() noexcept = 0;
    };

    [[nodiscard]] static float ApplyToHealthHierarchy(
            GameObject& target,
            GameObject* instigator,
            float rawDamage,
            IHurtboxNotifier* hurtboxNotifier) noexcept;
};

}  // namespace Spark
