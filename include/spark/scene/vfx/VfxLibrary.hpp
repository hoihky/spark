#pragma once

#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"

#include <cstdint>

namespace Spark {

enum class VfxBuiltinId : std::uint8_t {
    Fire = 0,
    Snow,
    Smoke,
    Sparkle,
    Explosion,
    Impact,
    Rain,
    MuzzleFlash,
    Blood,
    Heal,
    LevelUp,
    Electric,
    Poison,
    Dust,
    DashTrail,
    Aura,
    Soul,
    LootSparkle,
    WaterSplash,
    Leaves,
    MagicBolt,
    IceShatter,
    LaserHit,
    Embers,
    HolyLight,
    Curse,
    RocketTrail,
    GroundFire,
    Shockwave,
    MeteorTrail,
    Count
};

/** Built-in particle presets. See docs/VFX_ROADMAP.md. */
class VfxLibrary {
public:
    static void ApplyBuiltin(VfxBuiltinId id, ParticleEmitterComponent& emitter);

    [[nodiscard]] static const char* GetBuiltinName(VfxBuiltinId id) noexcept;

    /** Default one-shot burst size when <c>burst</c> is omitted in <c>.sparkvfx</c> (0 = continuous). */
    [[nodiscard]] static std::uint32_t GetDefaultBurstCount(VfxBuiltinId id) noexcept;

    /** Case-insensitive match on GetBuiltinName (e.g. "explosion"). */
    [[nodiscard]] static bool TryApplyBuiltinByName(const char* name, ParticleEmitterComponent& emitter);

    [[nodiscard]] static bool IsBuiltinName(const char* name) noexcept;
};

}  // namespace Spark
