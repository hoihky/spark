#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

void ParticleEmitterComponent::SetMaxParticles(std::uint32_t n) noexcept {
    maxParticles = std::max(1u, n);
    slots.Clear();
}

void ParticleEmitterComponent::SetLifetime(float minSec, float maxSec) noexcept {
    lifeMin = std::max(0.01F, minSec);
    lifeMax = std::max(lifeMin, maxSec);
}

void ParticleEmitterComponent::SetStartEndSize(float start, float end) noexcept {
    sizeStart = std::max(0.001F, start);
    sizeEnd = std::max(0.001F, end);
}

void ParticleEmitterComponent::SetStartEndColor(const Vector4& start, const Vector4& end) noexcept {
    colorStart = start;
    colorEnd = end;
}

void ParticleEmitterComponent::SetEmissionDirection(const Vector3& dir) noexcept {
    emissionDir = dir.LengthSquared() > 1.0e-8F ? dir.Normalized() : Vector3{0.0F, 1.0F, 0.0F};
}

void ParticleEmitterComponent::SetSpeedRange(float minS, float maxS) noexcept {
    speedMin = std::max(0.0F, minS);
    speedMax = std::max(speedMin, maxS);
}

float ParticleEmitterComponent::Random01() noexcept {
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    const std::uint32_t u = rng;
    return static_cast<float>(u & 0xffffffu) / static_cast<float>(0x1000000u);
}

Vector3 ParticleEmitterComponent::RandomUnitSphere() noexcept {
    const float z = Random01() * 2.0F - 1.0F;
    const float t = Random01() * TwoPi;
    const float r = std::sqrt(std::max(0.0F, 1.0F - z * z));
    return {r * std::cos(t), r * std::sin(t), z};
}

void ParticleEmitterComponent::EnsureSlotCapacity() {
    if (slots.GetSize() < static_cast<std::size_t>(maxParticles)) {
        slots.Resize(static_cast<std::size_t>(maxParticles));
    }
}

void ParticleEmitterComponent::SpawnOne(const Vector3& origin) {
    EnsureSlotCapacity();
    const std::uint32_t cap = static_cast<std::uint32_t>(slots.GetSize());
    for (std::uint32_t i = 0; i < cap; ++i) {
        if (!slots[i].alive) {
            SimParticle& p = slots[i];
            p.alive = true;
            p.position = origin;
            p.age = 0.0F;
            p.maxAge = lifeMin + Random01() * (lifeMax - lifeMin);
            p.size0 = sizeStart;
            p.size1 = sizeEnd;
            p.color0 = colorStart;
            p.color1 = colorEnd;
            const Vector3 basis = emissionDir;
            const Vector3 jitter = RandomUnitSphere() * spreadRadians;
            Vector3 dir = basis + jitter;
            if (dir.LengthSquared() < 1.0e-8F) {
                dir = basis;
            } else {
                dir = dir.Normalized();
            }
            const float sp = speedMin + Random01() * (speedMax - speedMin);
            p.velocity = dir * sp;
            break;
        }
    }
}

void ParticleEmitterComponent::OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext&) {
    if (!enabled) {
        return;
    }
    const float dt = timing.deltaTimeSeconds;
    if (dt <= 0.0F) {
        return;
    }

    EnsureSlotCapacity();
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector3 origin{wm.m[12], wm.m[13], wm.m[14]};

    spawnDebt += emissionRate * dt;
    while (spawnDebt >= 1.0F) {
        spawnDebt -= 1.0F;
        SpawnOne(origin);
    }

    const std::uint32_t cap = static_cast<std::uint32_t>(slots.GetSize());
    for (std::uint32_t i = 0; i < cap; ++i) {
        SimParticle& p = slots[i];
        if (!p.alive) {
            continue;
        }
        p.age += dt;
        if (p.age >= p.maxAge) {
            p.alive = false;
            continue;
        }
        p.velocity += gravity * dt;
        p.position += p.velocity * dt;
    }
}

void ParticleEmitterComponent::CollectInstances(Array<SceneParticleInstance>& out) const {
    if (slots.IsEmpty()) {
        return;
    }
    const std::uint32_t n = static_cast<std::uint32_t>(slots.GetSize());
    for (std::uint32_t i = 0; i < n; ++i) {
        const SimParticle& p = slots[i];
        if (!p.alive) {
            continue;
        }
        const float t = p.maxAge > 1.0e-6F ? (p.age / p.maxAge) : 1.0F;
        const float u = std::clamp(t, 0.0F, 1.0F);
        const float sz = p.size0 + (p.size1 - p.size0) * u;
        const Vector4 c = p.color0 + (p.color1 - p.color0) * u;
        SceneParticleInstance inst{};
        inst.position = p.position;
        inst.size = sz;
        inst.color = c;
        out.PushBack(inst);
    }
}

}  // namespace Spark
