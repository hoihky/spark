#include "spark/demo/platformer2d/Platformer2DExplosionFx.hpp"

#include "spark/ecs/components/rendering/BlendModeComponent.hpp"
#include "spark/render/scene/SceneBlendMode.hpp"

#include <algorithm>
#include <cmath>

namespace Spark::Platformer2D {

namespace {

[[nodiscard]] float Hash01(std::uint32_t& state) noexcept
{
    state = state * 1664525U + 1013904223U;
    return static_cast<float>(state & 0x00FFFFFFU) / static_cast<float>(0x01000000U);
}

}  // namespace

void ExplosionFx::Deactivate(Particle& particle) noexcept
{
    particle.active = false;
    particle.age = 0.0F;
    particle.lifetime = 0.0F;
    if (particle.tr != nullptr) {
        particle.tr->SetTranslation({-140.0F, -140.0F, 0.0F});
    }
    if (particle.spr != nullptr) {
        particle.spr->SetTint({1.0F, 0.55F, 0.18F, 0.0F});
    }
}

void ExplosionFx::Initialize(
        Spark::GameWorld& world,
        const Spark::SharedPtr<Spark::Texture2D>& texture,
        const int maxParticles,
        const int sortOrderBase,
        Spark::DemoRootCollection& roots)
{
    Shutdown(world);
    particles.Clear();
    particles.Resize(static_cast<std::size_t>(maxParticles));
    for (int pi = 0; pi < maxParticles; ++pi) {
        Spark::GameObject* go = world.CreateGameObject();
        go->GetName() = Spark::Utf8String("PlatExplosionParticle");
        Spark::TransformComponent* tr = go->AddComponent<Spark::TransformComponent>();
        tr->SetTranslation({-140.0F, -140.0F, 0.0F});
        go->AddComponent<Spark::BlendModeComponent>(Spark::SceneBlendMode::Additive);
        Spark::SpriteComponent* spr = go->AddComponent<Spark::SpriteComponent>(
                texture,
                Spark::Vector4{1.0F, 0.62F, 0.22F, 0.0F},
                Spark::Vector4{0.0F, 0.0F, 1.0F, 1.0F},
                sortOrderBase + pi);
        particles[static_cast<std::size_t>(pi)] = {false, go, tr, spr, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.18F};
        roots.Track(go);
    }
}

void ExplosionFx::Shutdown(Spark::GameWorld& /*world*/) noexcept
{
    particles.Clear();
}

void ExplosionFx::SpawnBurst(const float worldX, const float worldY, const int particleCount) noexcept
{
    int spawned = 0;
    for (std::size_t pi = 0; pi < particles.GetSize() && spawned < particleCount; ++pi) {
        Particle& particle = particles[pi];
        if (particle.active) {
            continue;
        }
        const float angle = Hash01(rng) * 6.2831853F;
        const float speed = 2.2F + Hash01(rng) * 4.8F;
        particle.active = true;
        particle.x = worldX;
        particle.y = worldY;
        particle.vx = std::cos(angle) * speed;
        particle.vy = std::sin(angle) * speed;
        particle.age = 0.0F;
        particle.lifetime = 0.28F + Hash01(rng) * 0.22F;
        particle.startScale = 0.12F + Hash01(rng) * 0.14F;
        if (particle.tr != nullptr) {
            particle.tr->SetTranslation({particle.x, particle.y, 0.07F});
            particle.tr->SetUniformScale(particle.startScale);
        }
        if (particle.spr != nullptr) {
            const float hue = Hash01(rng);
            particle.spr->SetTint(
                    {0.95F + 0.05F * hue,
                     0.35F + 0.35F * (1.0F - hue),
                     0.12F + 0.18F * hue,
                     0.95F});
        }
        ++spawned;
    }
}

void ExplosionFx::Tick(const float deltaSeconds) noexcept
{
    for (std::size_t pi = 0; pi < particles.GetSize(); ++pi) {
        Particle& particle = particles[pi];
        if (!particle.active) {
            continue;
        }
        particle.age += deltaSeconds;
        if (particle.age >= particle.lifetime) {
            Deactivate(particle);
            continue;
        }
        const float t = particle.age / particle.lifetime;
        particle.x += particle.vx * deltaSeconds;
        particle.y += particle.vy * deltaSeconds;
        particle.vy -= 6.5F * deltaSeconds;
        if (particle.tr != nullptr) {
            particle.tr->SetTranslation({particle.x, particle.y, 0.07F});
            particle.tr->SetUniformScale(particle.startScale * (1.0F + t * 1.8F));
        }
        if (particle.spr != nullptr) {
            const float fade = 1.0F - t * t;
            const Spark::Vector4 tint = particle.spr->GetTint();
            particle.spr->SetTint({tint.x, tint.y, tint.z, 0.95F * fade});
        }
    }
}

}  // namespace Spark::Platformer2D
