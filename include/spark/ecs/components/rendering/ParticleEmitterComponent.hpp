#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/vfx/ParticleCurves.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class Texture2D;
struct SceneParticleInstance;  // SceneRenderParams.hpp

/** Where simulated particles are drawn (camera billboards vs sorted sprite quads). */
enum class ParticleRenderSpace : std::uint8_t {
    CameraBillboard = 0,
    SpriteLayer = 1,
};

/**
 * CPU particle emitter: spawns, simulates, and exposes instances for SceneRenderParams::particles.
 * Expects a TransformComponent on the same GameObject (emission origin = world translation).
 */
class ParticleEmitterComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::ParticleEmitter;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(is_emitter_enabled)
    [[nodiscard]] bool IsEmitterEnabled() const noexcept { return enabled; }
    SPARK_SCRIPT_BIND(set_emitter_enabled)
    void SetEmitterEnabled(bool e) noexcept { enabled = e; }

    void SetMaxParticles(std::uint32_t n) noexcept;
    SPARK_SCRIPT_BIND(get_max_particles)
    [[nodiscard]] std::uint32_t GetMaxParticles() const noexcept { return maxParticles; }

    /** Particles spawned per second (fractional accumulation). */
    SPARK_SCRIPT_BIND(set_emission_rate)
    void SetEmissionRate(float rate) noexcept { emissionRate = rate; }
    SPARK_SCRIPT_BIND(get_emission_rate)
    [[nodiscard]] float GetEmissionRate() const noexcept { return emissionRate; }

    SPARK_SCRIPT_BIND(set_lifetime)
    void SetLifetime(float minSec, float maxSec) noexcept;
    SPARK_SCRIPT_BIND(set_start_end_size)
    void SetStartEndSize(float start, float end) noexcept;
    SPARK_SCRIPT_BIND(set_start_end_color)
    void SetStartEndColor(const Vector4& start, const Vector4& end) noexcept;
    SPARK_SCRIPT_BIND(set_gravity)
    void SetGravity(const Vector3& g) noexcept { gravity = g; }
    /** Emission direction in local or world space (see SetUseLocalEmission). */
    SPARK_SCRIPT_BIND(set_emission_direction)
    void SetEmissionDirection(const Vector3& dir) noexcept;
    SPARK_SCRIPT_BIND(get_emission_direction)
    [[nodiscard]] const Vector3& GetEmissionDirection() const noexcept { return emissionDir; }
    /** When true, emission direction is rotated by the owner's TransformComponent. */
    SPARK_SCRIPT_BIND(set_use_local_emission)
    void SetUseLocalEmission(bool local) noexcept { useLocalEmission = local; }
    SPARK_SCRIPT_BIND(get_use_local_emission)
    [[nodiscard]] bool GetUseLocalEmission() const noexcept { return useLocalEmission; }
    /** Half-angle cone around emission direction (radians). */
    SPARK_SCRIPT_BIND(set_spread_angle_radians)
    void SetSpreadAngleRadians(float rad) noexcept { spreadRadians = rad; }
    SPARK_SCRIPT_BIND(set_speed_range)
    void SetSpeedRange(float minS, float maxS) noexcept;

    SPARK_SCRIPT_BIND(get_lifetime_min)
    [[nodiscard]] float GetLifetimeMin() const noexcept { return lifeMin; }
    SPARK_SCRIPT_BIND(get_lifetime_max)
    [[nodiscard]] float GetLifetimeMax() const noexcept { return lifeMax; }
    SPARK_SCRIPT_BIND(get_start_size)
    [[nodiscard]] float GetStartSize() const noexcept { return sizeStart; }
    SPARK_SCRIPT_BIND(get_end_size)
    [[nodiscard]] float GetEndSize() const noexcept { return sizeEnd; }
    SPARK_SCRIPT_BIND(get_color_start)
    [[nodiscard]] const Vector4& GetColorStart() const noexcept { return colorStart; }
    SPARK_SCRIPT_BIND(get_color_end)
    [[nodiscard]] const Vector4& GetColorEnd() const noexcept { return colorEnd; }
    SPARK_SCRIPT_BIND(get_gravity)
    [[nodiscard]] const Vector3& GetGravity() const noexcept { return gravity; }
    SPARK_SCRIPT_BIND(get_spread_angle_radians)
    [[nodiscard]] float GetSpreadAngleRadians() const noexcept { return spreadRadians; }
    SPARK_SCRIPT_BIND(get_speed_min)
    [[nodiscard]] float GetSpeedMin() const noexcept { return speedMin; }
    SPARK_SCRIPT_BIND(get_speed_max)
    [[nodiscard]] float GetSpeedMax() const noexcept { return speedMax; }

    SPARK_SCRIPT_BIND(get_size_curve)
    [[nodiscard]] const ParticleFloatCurve& GetSizeCurve() const noexcept { return sizeCurve; }
    SPARK_SCRIPT_BIND(get_color_curve)
    [[nodiscard]] const ParticleColorCurve& GetColorCurve() const noexcept { return colorCurve; }
    SPARK_SCRIPT_BIND(set_size_curve)
    void SetSizeCurve(const ParticleFloatCurve& curve) noexcept;
    SPARK_SCRIPT_BIND(set_color_curve)
    void SetColorCurve(const ParticleColorCurve& curve) noexcept;

    SPARK_SCRIPT_BIND(set_emission_module_id)
    void SetEmissionModuleId(const char* moduleId) noexcept;
    SPARK_SCRIPT_BIND(get_emission_module_id)
    [[nodiscard]] const char* GetEmissionModuleId() const noexcept { return emissionModuleId.CStr(); }

    SPARK_SCRIPT_BIND(set_ring_radius)
    void SetRingRadius(float radius) noexcept { ringRadius = radius < 0.0F ? 0.0F : radius; }
    SPARK_SCRIPT_BIND(get_ring_radius)
    [[nodiscard]] float GetRingRadius() const noexcept { return ringRadius; }

    void SetTexture(SharedPtr<Texture2D> texture) noexcept { particleTexture = MoveTemp(texture); }
    [[nodiscard]] const SharedPtr<Texture2D>& GetTexture() const noexcept { return particleTexture; }
    SPARK_SCRIPT_BIND(set_uv_rect)
    void SetUvRect(const Vector4& rect) noexcept { uvRect = rect; }
    SPARK_SCRIPT_BIND(get_uv_rect)
    [[nodiscard]] const Vector4& GetUvRect() const noexcept { return uvRect; }

    SPARK_SCRIPT_BIND(set_render_space)
    void SetRenderSpace(ParticleRenderSpace space) noexcept { renderSpace = space; }
    SPARK_SCRIPT_BIND(get_render_space)
    [[nodiscard]] ParticleRenderSpace GetRenderSpace() const noexcept { return renderSpace; }
    /** Drawable sort key when <c>RenderSpace::SpriteLayer</c> (uses owner render layer if unset). */
    SPARK_SCRIPT_BIND(set_sprite_layer_sort_order)
    void SetSpriteLayerSortOrder(std::int32_t order) noexcept { spriteLayerSortOrder = order; }
    SPARK_SCRIPT_BIND(get_sprite_layer_sort_order)
    [[nodiscard]] std::int32_t GetSpriteLayerSortOrder() const noexcept { return spriteLayerSortOrder; }

    /** Append living particles for the renderer (respects global cap via caller). */
    void CollectInstances(Array<SceneParticleInstance>& out) const;

    /** Spawn up to count particles immediately at the owner's world origin. */
    void Burst(GameObject& owner, std::uint32_t count);

    /** Clears all living particles and spawn debt (for pooled reuse). */
    SPARK_SCRIPT_BIND(clear_particles)
    void ClearParticles() noexcept;

    SPARK_SCRIPT_BIND(get_alive_particle_count)
    [[nodiscard]] std::uint32_t GetAliveParticleCount() const noexcept;

    /** Used by built-in emission modules. */
    SPARK_SCRIPT_BIND(emit_continuous)
    void EmitContinuous(const Vector3& origin, const Vector3& worldEmissionDir, float deltaTimeSeconds);
    SPARK_SCRIPT_BIND(emit_ring)
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
    void SpawnOneWithBasis(const Vector3& origin, const Vector3& basisDir, float spreadScale) noexcept;
    void BurstRing(GameObject& owner, std::uint32_t count, const Vector3& origin);
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
    ParticleRenderSpace renderSpace = ParticleRenderSpace::CameraBillboard;
    std::int32_t spriteLayerSortOrder = 200;

    Array<SimParticle> slots{};
    std::uint32_t rng = 0xC0FFEEu;
};

}  // namespace Spark
