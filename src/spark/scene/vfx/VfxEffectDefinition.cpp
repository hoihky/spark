#include "spark/scene/vfx/VfxEffectDefinition.hpp"

#include <algorithm>

#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"
#include "spark/scene/vfx/VfxLibrary.hpp"

namespace Spark {

void VfxEmitterSpec::ApplyTo(ParticleEmitterComponent& emitterOut) const {
    if (UsesBuiltin()) {
        VfxLibrary::TryApplyBuiltinByName(builtinName.CStr(), emitterOut);
    } else {
        emitter.ApplyTo(emitterOut);
    }
    if (applyEmitterOverrides) {
        emitter.ApplyTo(emitterOut);
    }
}

void VfxEmitterSpec::Activate(
        ParticleEmitterComponent& emitterOut,
        GameObject& owner,
        const VfxPlaybackMode mode) const {
    ApplyTo(emitterOut);
    emitterOut.SetEmitterEnabled(true);
    if (burstCount > 0) {
        emitterOut.Burst(owner, burstCount);
    } else if (mode == VfxPlaybackMode::Once && emitterOut.GetEmissionRate() <= 0.0F) {
        (void)owner;
    }
}

float VfxEffectDefinition::GetEstimatedDurationSeconds() const noexcept {
    float maxEnd = 0.0F;
    for (std::size_t i = 0; i < emitters.GetSize(); ++i) {
        const VfxEmitterSpec& spec = emitters[i];
        const float end = spec.startTimeSeconds + std::max(spec.durationSeconds, 0.45F);
        if (end > maxEnd) {
            maxEnd = end;
        }
    }
    return maxEnd;
}

VfxAsset VfxEffectDefinition::ToAsset() const {
    VfxAsset asset{};
    asset.definition = *this;
    return asset;
}

VfxEffectDefinition VfxEffectDefinition::Fireworks() {
    VfxEffectDefinition definition{};

    VfxEmitterSpec trail{};
    trail.startTimeSeconds = 0.0F;
    trail.durationSeconds = 0.28F;
    trail.builtinName = Utf8String("rocket_trail");
    trail.applyEmitterOverrides = true;
    trail.emitter.emissionRate = 165.0F;
    trail.emitter.lifeMin = 0.1F;
    trail.emitter.lifeMax = 0.28F;
    trail.emitter.speedMin = 5.0F;
    trail.emitter.speedMax = 9.0F;
    trail.emitter.spreadRadians = 0.18F;
    trail.emitter.useLocalEmission = true;
    definition.emitters.PushBack(trail);

    VfxEmitterSpec burst{};
    burst.startTimeSeconds = 0.28F;
    burst.builtinName = Utf8String("explosion");
    burst.burstCount = 96;
    definition.emitters.PushBack(burst);

    VfxEmitterSpec sparkle{};
    sparkle.startTimeSeconds = 0.42F;
    sparkle.builtinName = Utf8String("sparkle");
    sparkle.burstCount = 48;
    definition.emitters.PushBack(sparkle);

    return definition;
}

VfxEffectDefinition VfxEffectDefinition::Confetti() {
    VfxEffectDefinition definition{};

    VfxEmitterSpec pop{};
    pop.startTimeSeconds = 0.0F;
    pop.builtinName = Utf8String("sparkle");
    pop.burstCount = 144;
    pop.applyEmitterOverrides = true;
    pop.emitter.gravity = {0.0F, -3.8F, 0.0F};
    pop.emitter.spreadRadians = 2.5F;
    pop.emitter.speedMin = 2.8F;
    pop.emitter.speedMax = 8.5F;
    pop.emitter.lifeMin = 0.5F;
    pop.emitter.lifeMax = 1.9F;
    pop.emitter.useLocalEmission = true;
    definition.emitters.PushBack(pop);

    VfxEmitterSpec drift{};
    drift.startTimeSeconds = 0.06F;
    drift.durationSeconds = 0.55F;
    drift.builtinName = Utf8String("snow");
    drift.applyEmitterOverrides = true;
    drift.emitter.emissionRate = 64.0F;
    drift.emitter.gravity = {0.0F, -1.2F, 0.0F};
    drift.emitter.spreadRadians = 1.75F;
    definition.emitters.PushBack(drift);

    return definition;
}

VfxEffectDefinition VfxEffectDefinition::MeteorStrike() {
    VfxEffectDefinition definition{};

    VfxEmitterSpec trail{};
    trail.startTimeSeconds = 0.0F;
    trail.durationSeconds = 0.38F;
    trail.builtinName = Utf8String("meteor_trail");
    definition.emitters.PushBack(trail);

    VfxEmitterSpec impact{};
    impact.startTimeSeconds = 0.36F;
    impact.builtinName = Utf8String("explosion");
    impact.burstCount = 88;
    definition.emitters.PushBack(impact);

    VfxEmitterSpec smoke{};
    smoke.startTimeSeconds = 0.42F;
    smoke.durationSeconds = 0.95F;
    smoke.builtinName = Utf8String("smoke");
    smoke.applyEmitterOverrides = true;
    smoke.emitter.emissionRate = 72.0F;
    definition.emitters.PushBack(smoke);

    return definition;
}

VfxEffectDefinition VfxEffectDefinition::MagicImpact() {
    VfxEffectDefinition definition{};

    VfxEmitterSpec bolt{};
    bolt.startTimeSeconds = 0.0F;
    bolt.durationSeconds = 0.12F;
    bolt.builtinName = Utf8String("magic_bolt");
    definition.emitters.PushBack(bolt);

    VfxEmitterSpec burst{};
    burst.startTimeSeconds = 0.1F;
    burst.builtinName = Utf8String("sparkle");
    burst.burstCount = 56;
    definition.emitters.PushBack(burst);

    VfxEmitterSpec arc{};
    arc.startTimeSeconds = 0.14F;
    arc.builtinName = Utf8String("electric");
    arc.burstCount = 32;
    definition.emitters.PushBack(arc);

    return definition;
}

VfxEffectDefinition VfxEffectDefinition::SmokeGrenade() {
    VfxEffectDefinition definition{};

    VfxEmitterSpec pop{};
    pop.startTimeSeconds = 0.0F;
    pop.builtinName = Utf8String("dust");
    pop.burstCount = 28;
    definition.emitters.PushBack(pop);

    VfxEmitterSpec cloud{};
    cloud.startTimeSeconds = 0.05F;
    cloud.durationSeconds = 2.4F;
    cloud.builtinName = Utf8String("smoke");
    cloud.applyEmitterOverrides = true;
    cloud.emitter.emissionRate = 68.0F;
    cloud.emitter.lifeMin = 1.2F;
    cloud.emitter.lifeMax = 2.8F;
    cloud.emitter.spreadRadians = 1.35F;
    definition.emitters.PushBack(cloud);

    return definition;
}

}  // namespace Spark
