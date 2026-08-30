#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Constants.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/vfx/modules/ParticleModuleRegistry.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Spark {

void ParticleEmitterComponent::SetMaxParticles(std::uint32_t n) noexcept {
    maxParticles = std::max(1u, n);
    slots.Clear();
}

void ParticleEmitterComponent::SetLifetime(float minSec, float maxSec) noexcept {
    lifeMin = std::max(0.01F, minSec);
    lifeMax = std::max(lifeMin, maxSec);
}

void ParticleEmitterComponent::SetStartEndSize(const float start, const float end) noexcept {
    sizeStart = std::max(0.001F, start);
    sizeEnd = std::max(0.001F, end);
    sizeCurve.SetEndpoints(sizeStart, sizeEnd);
}

void ParticleEmitterComponent::SetStartEndColor(const Vector4& start, const Vector4& end) noexcept {
    colorStart = start;
    colorEnd = end;
    colorCurve.SetEndpoints(colorStart, colorEnd);
}

void ParticleEmitterComponent::SetSizeCurve(const ParticleFloatCurve& curve) noexcept {
    sizeCurve = curve;
    if (sizeCurve.keyframeCount >= 2) {
        sizeStart = sizeCurve.keyframes[0].value;
        sizeEnd = sizeCurve.keyframes[sizeCurve.keyframeCount - 1].value;
    }
}

void ParticleEmitterComponent::SetColorCurve(const ParticleColorCurve& curve) noexcept {
    colorCurve = curve;
    if (colorCurve.keyframeCount >= 2) {
        colorStart = colorCurve.keyframes[0].color;
        colorEnd = colorCurve.keyframes[colorCurve.keyframeCount - 1].color;
    }
}

void ParticleEmitterComponent::SetEmissionModuleId(const char* const moduleId) noexcept {
    emissionModuleId = Utf8String(moduleId != nullptr ? moduleId : ParticleModuleRegistry::DefaultModuleId());
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

Vector3 ParticleEmitterComponent::ResolveEmissionDirection(const GameObject& owner) const {
    Vector3 dir = emissionDir;
    if (useLocalEmission) {
        const Matrix4 wm = owner.GetWorldMatrix();
        dir = {
            wm.m[0] * emissionDir.x + wm.m[4] * emissionDir.y + wm.m[8] * emissionDir.z,
            wm.m[1] * emissionDir.x + wm.m[5] * emissionDir.y + wm.m[9] * emissionDir.z,
            wm.m[2] * emissionDir.x + wm.m[6] * emissionDir.y + wm.m[10] * emissionDir.z,
        };
    }
    if (dir.LengthSquared() < 1.0e-8F) {
        dir = Vector3{0.0F, 1.0F, 0.0F};
    }
    return dir.Normalized();
}

void ParticleEmitterComponent::SpawnOne(const Vector3& origin, const Vector3& worldEmissionDir) {
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
            const Vector3 basis = worldEmissionDir;
            const Vector3 jitter = RandomUnitSphere() * spreadRadians;
            Vector3 dir = basis + jitter;
            if (dir.LengthSquared() < 1.0e-8F) {
                dir = basis;
            } else {
                dir = dir.Normalized();
            }
            const float sp = speedMin + Random01() * (speedMax - speedMin);
            p.velocity = dir * sp;
            return;
        }
    }
}

void ParticleEmitterComponent::EmitContinuous(
        const Vector3& origin,
        const Vector3& worldEmissionDir,
        const float deltaTimeSeconds) {
    spawnDebt += emissionRate * deltaTimeSeconds;
    while (spawnDebt >= 1.0F) {
        spawnDebt -= 1.0F;
        SpawnOne(origin, worldEmissionDir);
    }
}

void ParticleEmitterComponent::EmitRing(
        const Vector3& origin,
        const Vector3& worldEmissionDir,
        const float deltaTimeSeconds) {
    spawnDebt += emissionRate * deltaTimeSeconds;
    while (spawnDebt >= 1.0F) {
        spawnDebt -= 1.0F;
        const float angle = Random01() * TwoPi;
        Vector3 ringOrigin = origin;
        ringOrigin.x += std::cos(angle) * ringRadius;
        ringOrigin.z += std::sin(angle) * ringRadius;
        SpawnOne(ringOrigin, worldEmissionDir);
    }
}

void ParticleEmitterComponent::Burst(GameObject& owner, const std::uint32_t count) {
    if (!enabled || count == 0) {
        return;
    }
    const Matrix4 wm = owner.GetWorldMatrix();
    const Vector3 origin{wm.m[12], wm.m[13], wm.m[14]};
    const Vector3 worldDir = ResolveEmissionDirection(owner);
    for (std::uint32_t i = 0; i < count; ++i) {
        SpawnOne(origin, worldDir);
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
    const Vector3 worldDir = ResolveEmissionDirection(owner);

    ParticleModuleRegistry::EmitFrame(*this, owner, origin, worldDir, dt);

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

void ParticleEmitterComponent::ClearParticles() noexcept {
    for (std::size_t i = 0; i < slots.GetSize(); ++i) {
        slots[i].alive = false;
    }
    spawnDebt = 0.0F;
}

std::uint32_t ParticleEmitterComponent::GetAliveParticleCount() const noexcept {
    std::uint32_t count = 0;
    const std::uint32_t n = static_cast<std::uint32_t>(slots.GetSize());
    for (std::uint32_t i = 0; i < n; ++i) {
        if (slots[i].alive) {
            ++count;
        }
    }
    return count;
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
        const float sz = sizeCurve.IsEmpty() ? (p.size0 + (p.size1 - p.size0) * u) : sizeCurve.Evaluate(u);
        const Vector4 c = colorCurve.IsEmpty() ? (p.color0 + (p.color1 - p.color0) * u) : colorCurve.Evaluate(u);
        SceneParticleInstance inst{};
        inst.position = p.position;
        inst.size = sz;
        inst.color = c;
        inst.uvRect = uvRect;
        out.PushBack(inst);
    }
}

}  // namespace Spark
