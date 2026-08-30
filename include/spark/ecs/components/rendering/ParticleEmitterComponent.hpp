#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/vfx/ParticleCurves.hpp"

namespace Spark {

class GameObject;
class Texture2D;
struct SceneParticleInstance;  // SceneRenderParams.hpp

/**
 * CPU particle emitter: spawns, simulates, and exposes instances for SceneRenderParams::particles.
 * Expects a TransformComponent on the same GameObject (emission origin = world translation).
 */
class ParticleEmitterComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::ParticleEmitter;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    [[nodiscard]] bool IsEmitterEnabled() const noexcept { return enabled; }
    void SetEmitterEnabled(bool e) noexcept { enabled = e; }

    void SetMaxParticles(std::uint32_t n) noexcept;
    [[nodiscard]] std::uint32_t GetMaxParticles() const noexcept { return maxParticles; }

    /** Particles spawned per second (fractional accumulation). */
    void SetEmissionRate(float rate) noexcept { emissionRate = rate; }
    [[nodiscard]] float GetEmissionRate() const noexcept { return emissionRate; }

    void SetLifetime(float minSec, float maxSec) noexcept;
    void SetStartEndSize(float start, float end) noexcept;
    void SetStartEndColor(const Vector4& start, const Vector4& end) noexcept;
    void SetGravity(const Vector3& g) noexcept { gravity = g; }
    /** Emission direction in local or world space (see SetUseLocalEmission). */
    void SetEmissionDirection(const Vector3& dir) noexcept;
    [[nodiscard]] const Vector3& GetEmissionDirection() const noexcept { return emissionDir; }
    /** When true, emission direction is rotated by the owner's TransformComponent. */
    void SetUseLocalEmission(bool local) noexcept { useLocalEmission = local; }
    [[nodiscard]] bool GetUseLocalEmission() const noexcept { return useLocalEmission; }
    /** Half-angle cone around emission direction (radians). */
    void SetSpreadAngleRadians(float rad) noexcept { spreadRadians = rad; }
    void SetSpeedRange(float minS, float maxS) noexcept;

    [[nodiscard]] float GetLifetimeMin() const noexcept { return lifeMin; }
    [[nodiscard]] float GetLifetimeMax() const noexcept { return lifeMax; }
    [[nodiscard]] float GetStartSize() const noexcept { return sizeStart; }
    [[nodiscard]] float GetEndSize() const noexcept { return sizeEnd; }
    [[nodiscard]] const Vector4& GetColorStart() const noexcept { return colorStart; }
    [[nodiscard]] const Vector4& GetColorEnd() const noexcept { return colorEnd; }
    [[nodiscard]] const Vector3& GetGravity() const noexcept { return gravity; }
    [[nodiscard]] float GetSpreadAngleRadians() const noexcept { return spreadRadians; }
    [[nodiscard]] float GetSpeedMin() const noexcept { return speedMin; }
    [[nodiscard]] float GetSpeedMax() const noexcept { return speedMax; }

    [[nodiscard]] const ParticleFloatCurve& GetSizeCurve() const noexcept { return sizeCurve; }
    [[nodiscard]] const ParticleColorCurve& GetColorCurve() const noexcept { return colorCurve; }
    void SetSizeCurve(const ParticleFloatCurve& curve) noexcept;
    void SetColorCurve(const ParticleColorCurve& curve) noexcept;

    void SetEmissionModuleId(const char* moduleId) noexcept;
    [[nodiscard]] const char* GetEmissionModuleId() const noexcept { return emissionModuleId.CStr(); }

    void SetRingRadius(float radius) noexcept { ringRadius = radius < 0.0F ? 0.0F : radius; }
    [[nodiscard]] float GetRingRadius() const noexcept { return ringRadius; }

    void SetTexture(SharedPtr<Texture2D> texture) noexcept { particleTexture = MoveTemp(texture); }
    [[nodiscard]] const SharedPtr<Texture2D>& GetTexture() const noexcept { return particleTexture; }
    void SetUvRect(const Vector4& rect) noexcept { uvRect = rect; }
    [[nodiscard]] const Vector4& GetUvRect() const noexcept { return uvRect; }

    /** Append living particles for the renderer (respects global cap via caller). */
    void CollectInstances(Array<SceneParticleInstance>& out) const;

    /** Spawn up to count particles immediately at the owner's world origin. */
    void Burst(GameObject& owner, std::uint32_t count);

    /** Clears all living particles and spawn debt (for pooled reuse). */
    void ClearParticles() noexcept;

    [[nodiscard]] std::uint32_t GetAliveParticleCount() const noexcept;

    /** Used by built-in emission modules. */
    void EmitContinuous(const Vector3& origin, const Vector3& worldEmissionDir, float deltaTimeSeconds);
    void EmitRing(const Vector3& origin, const Vector3& worldEmissionDir, float deltaTimeSeconds);

private:
    struct SimParticle {
        bool alive = false;
        Vector3 position{};
        Vector3 velocity{};
        float age = 0.0F;
        float maxAge = 1.0F;
        float size0 = 0.1F;
        float size1 = 0.05F;
        Vector4 color0{1, 1, 1, 1};
        Vector4 color1{1, 1, 1, 0};
    };

    void EnsureSlotCapacity();
    void SpawnOne(const Vector3& origin, const Vector3& worldEmissionDir);
    [[nodiscard]] Vector3 ResolveEmissionDirection(const GameObject& owner) const;
    [[nodiscard]] float Random01() noexcept;
    [[nodiscard]] Vector3 RandomUnitSphere() noexcept;

    bool enabled = true;
    std::uint32_t maxParticles = 512;
    float emissionRate = 48.0F;
    float spawnDebt = 0.0F;
    float lifeMin = 0.8F;
    float lifeMax = 1.6F;
    float sizeStart = 0.14F;
    float sizeEnd = 0.02F;
    Vector4 colorStart{0.95F, 0.85F, 0.35F, 1.0F};
    Vector4 colorEnd{0.9F, 0.2F, 0.05F, 0.0F};
    ParticleFloatCurve sizeCurve{};
    ParticleColorCurve colorCurve{};
    Vector3 gravity{0.0F, -1.8F, 0.0F};
    Vector3 emissionDir{0.0F, 1.0F, 0.0F};
    bool useLocalEmission = false;
    float spreadRadians = 0.55F;
    float speedMin = 1.2F;
    float speedMax = 2.8F;
    Utf8String emissionModuleId{"continuous"};
    float ringRadius = 0.35F;
    SharedPtr<Texture2D> particleTexture{};
    Vector4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};

    Array<SimParticle> slots{};
    std::uint32_t rng = 0xC0FFEEu;
};

}  // namespace Spark
