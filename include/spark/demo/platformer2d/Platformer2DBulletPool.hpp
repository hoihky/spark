#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Function.hpp"
#include "spark/demo/DemoFoundation.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/Projectile2DComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/texture/Texture2D.hpp"

namespace Spark::Platformer2D {

/**
 * Strategy object describing how a pooled projectile should look and move.
 * Player and enemy bullets reuse the same pool implementation with different profiles.
 */
struct BulletProfile {
    float speed = 10.0F;
    float halfW = 0.05F;
    float halfH = 0.03F;
    float drawScale = 0.4F;
    float lifetime = 3.6F;
    float damage = 1.0F;
    Spark::Vector4 baseTint{1.0F, 1.0F, 1.0F, 1.0F};
    bool additiveBlend = true;
    Spark::Projectile2DShape projectileShape = Spark::Projectile2DShape::Box;
    Spark::PhysicsQueryFilter2D targetFilter{};
    bool blockOnSolidHit = true;
};

/**
 * Object-pool of lightweight sprite bullets backed by <c>Projectile2DComponent</c> for motion and hits.
 */
class BulletPool final {
public:
    struct Slot {
        bool active = false;
        Spark::GameObject* go = nullptr;
        Spark::TransformComponent* tr = nullptr;
        Spark::SpriteComponent* spr = nullptr;
        Spark::Projectile2DComponent* projectile = nullptr;
        float cx = 0.0F;
        float cy = 0.0F;
        float age = 0.0F;
        BulletProfile profile{};
    };

    using TargetHitHandler = Spark::Function<void(Slot& slot, Spark::GameObject& victim, float appliedDamage)>;

    void Initialize(
            Spark::GameWorld& world,
            const Spark::SharedPtr<Spark::Texture2D>& texture,
            int capacity,
            int sortOrderBase,
            Spark::Utf8String debugNamePrefix,
            Spark::DemoRootCollection& roots);

    void Shutdown(Spark::GameWorld& world) noexcept;

    void SetTargetHitHandler(TargetHitHandler handler) { onTargetHit = Spark::MoveTemp(handler); }

    [[nodiscard]] bool TrySpawn(
            float originX,
            float originY,
            float dirX,
            float dirY,
            Spark::GameObject* instigator,
            const BulletProfile& profile) noexcept;

    void Tick(
            float deltaSeconds,
            Spark::IEngineContext& context,
            float cullMinX,
            float cullMaxX,
            float cullMinY,
            float cullMaxY) noexcept;

    void DeactivateAll() noexcept;

    static void DeactivateSlot(Slot& slot) noexcept;

    [[nodiscard]] Spark::Array<Slot>& Slots() noexcept { return slots; }
    [[nodiscard]] const Spark::Array<Slot>& Slots() const noexcept { return slots; }

private:
    void ApplyProfileToProjectile(Spark::Projectile2DComponent& projectile, const BulletProfile& profile) noexcept;
    void WireProjectileCallbacks(std::size_t slotIndex) noexcept;

    Spark::Array<Slot> slots{};
    TargetHitHandler onTargetHit{};
};

}  // namespace Spark::Platformer2D
