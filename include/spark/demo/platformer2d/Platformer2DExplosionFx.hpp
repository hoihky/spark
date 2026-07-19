#pragma once

#include "spark/core/Array.hpp"
#include "spark/demo/DemoFoundation.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/SpriteComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Texture2D.hpp"

namespace Spark::Platformer2D {

/**
 * Pooled burst particles for enemy defeat feedback. Each explosion activates several short-lived sprites that
 * radiate outward — a lightweight stand-in for a full GPU particle system, easy to read in coursework.
 */
class ExplosionFx final {
public:
    struct Particle {
        bool active = false;
        Spark::GameObject* go = nullptr;
        Spark::TransformComponent* tr = nullptr;
        Spark::SpriteComponent* spr = nullptr;
        float x = 0.0F;
        float y = 0.0F;
        float vx = 0.0F;
        float vy = 0.0F;
        float age = 0.0F;
        float lifetime = 0.0F;
        float startScale = 0.18F;
    };

    void Initialize(
            Spark::GameWorld& world,
            const Spark::SharedPtr<Spark::Texture2D>& texture,
            int maxParticles,
            int sortOrderBase,
            Spark::DemoRootCollection& roots);

    void Shutdown(Spark::GameWorld& world) noexcept;

    void SpawnBurst(float worldX, float worldY, int particleCount) noexcept;

    void Tick(float deltaSeconds) noexcept;

private:
    static void Deactivate(Particle& particle) noexcept;

    Spark::Array<Particle> particles{};
    std::uint32_t rng = 0x51A0CE11U;
};

}  // namespace Spark::Platformer2D
