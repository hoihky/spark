#pragma once

#include "spark/core/Array.hpp"
#include "spark/demo/DemoFoundation.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Texture2D.hpp"

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
    Spark::Vector4 baseTint{1.0F, 1.0F, 1.0F, 1.0F};
    bool additiveBlend = true;
};

/**
 * Object-pool of lightweight sprite bullets. Avoids per-shot allocations and keeps Simulate() predictable.
 */
class BulletPool final {
public:
    struct Slot {
        bool active = false;
        Spark::GameObject* go = nullptr;
        Spark::TransformComponent* tr = nullptr;
        Spark::SpriteComponent* spr = nullptr;
        float cx = 0.0F;
        float cy = 0.0F;
        float vx = 0.0F;
        float vy = 0.0F;
        float age = 0.0F;
        float lifetime = 0.0F;
        BulletProfile profile{};
    };

    void Initialize(
            Spark::GameWorld& world,
            const Spark::SharedPtr<Spark::Texture2D>& texture,
            int capacity,
            int sortOrderBase,
            Spark::Utf8String debugNamePrefix,
            Spark::DemoRootCollection& roots);

    void Shutdown(Spark::GameWorld& world) noexcept;

    [[nodiscard]] bool TrySpawn(float originX, float originY, float dirX, float dirY, const BulletProfile& profile) noexcept;

    void Tick(float deltaSeconds, float cullMinX, float cullMaxX, float cullMinY, float cullMaxY) noexcept;

    void DeactivateAll() noexcept;

    static void DeactivateSlot(Slot& slot) noexcept;

    [[nodiscard]] Spark::Array<Slot>& Slots() noexcept { return slots; }
    [[nodiscard]] const Spark::Array<Slot>& Slots() const noexcept { return slots; }

private:
    Spark::Array<Slot> slots{};
};

}  // namespace Spark::Platformer2D
