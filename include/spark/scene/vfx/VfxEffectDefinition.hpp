#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/scene/vfx/VfxEmitterParams.hpp"

namespace Spark {

class GameObject;
class ParticleEmitterComponent;
class VfxAsset;

enum class VfxPlaybackMode : std::uint8_t {
    Continuous,
    Once,
};

/** One timed emitter layer inside a composite <c>VfxAsset</c>. */
struct VfxEmitterSpec {
    float startTimeSeconds = 0.0F;
    /** Continuous emission stops after this many seconds (0 = leave emission as configured). */
    float durationSeconds = 0.0F;
    Utf8String builtinName;
    VfxEmitterParams emitter{};
    std::uint32_t burstCount = 0;
    /** When set with a built-in, applies <c>emitter</c> after the preset. */
    bool applyEmitterOverrides = false;

    [[nodiscard]] bool UsesBuiltin() const noexcept { return !builtinName.IsEmpty(); }

    void ApplyTo(ParticleEmitterComponent& emitterOut) const;
    void Activate(ParticleEmitterComponent& emitterOut, GameObject& owner, VfxPlaybackMode mode) const;
};

/** Ordered multi-emitter effect (Composite pattern). Serialized in <c>.sparkvfx</c>. */
class VfxEffectDefinition {
public:
    Array<VfxEmitterSpec> emitters;
    /** Optional <c>.sparkscene</c> prefab spawned as a child hierarchy on play. */
    Utf8String prefabScenePath;

    [[nodiscard]] bool IsComposite() const noexcept { return !emitters.IsEmpty(); }
    [[nodiscard]] float GetEstimatedDurationSeconds() const noexcept;

    [[nodiscard]] VfxAsset ToAsset() const;
    [[nodiscard]] static VfxEffectDefinition Fireworks();
    [[nodiscard]] static VfxEffectDefinition Confetti();
    [[nodiscard]] static VfxEffectDefinition MeteorStrike();
    [[nodiscard]] static VfxEffectDefinition MagicImpact();
    [[nodiscard]] static VfxEffectDefinition SmokeGrenade();
};

}  // namespace Spark
