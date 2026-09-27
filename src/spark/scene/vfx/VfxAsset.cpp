#include "spark/scene/vfx/VfxAsset.hpp"

#include <algorithm>
#include <cstring>

#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/vfx/VfxAssetLoader.hpp"
#include "spark/scene/vfx/VfxEffectDefinition.hpp"
#include "spark/scene/vfx/VfxLibrary.hpp"

namespace Spark {

void VfxAsset::ApplyTo(ParticleEmitterComponent& emitterOut) const {
    if (!builtinName.IsEmpty()) {
        VfxLibrary::TryApplyBuiltinByName(builtinName.CStr(), emitterOut);
    } else {
        emitter.ApplyTo(emitterOut);
    }
}

void VfxAsset::Activate(
        ParticleEmitterComponent& emitterOut,
        GameObject& owner,
        const VfxPlaybackMode mode) const {
    ApplyTo(emitterOut);
    if (mode != VfxPlaybackMode::Once) {
        return;
    }
    std::uint32_t spawnBurst = burstCount;
    if (spawnBurst == 0 && !builtinName.IsEmpty()) {
        for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(VfxBuiltinId::Count); ++i) {
            const auto id = static_cast<VfxBuiltinId>(i);
            if (std::strcmp(builtinName.CStr(), VfxLibrary::GetBuiltinName(id)) == 0) {
                spawnBurst = VfxLibrary::GetDefaultBurstCount(id);
                break;
            }
        }
    }
    if (spawnBurst > 0) {
        emitterOut.Burst(owner, spawnBurst);
    } else if (std::strcmp(emitterOut.GetEmissionModuleId(), "continuous") == 0
               && emitterOut.GetEmissionRate() > 0.0F) {
        constexpr float kSeedSeconds = 0.6F;
        spawnBurst = static_cast<std::uint32_t>(emitterOut.GetEmissionRate() * kSeedSeconds + 0.5F);
        spawnBurst = std::min(spawnBurst, emitterOut.GetMaxParticles());
        spawnBurst = std::max(1U, spawnBurst);
        emitterOut.Burst(owner, spawnBurst);
    }
    // One-shots must not keep ring/continuous modules emitting forever.
    emitterOut.SetEmissionRate(0.0F);
}

bool VfxAsset::IsCompositeBuiltin(const char* builtinName) noexcept {
    if (builtinName == nullptr) {
        return false;
    }
    return std::strcmp(builtinName, "fireworks") == 0 || std::strcmp(builtinName, "confetti") == 0
        || std::strcmp(builtinName, "meteor_strike") == 0 || std::strcmp(builtinName, "magic_impact") == 0
        || std::strcmp(builtinName, "smoke_grenade") == 0;
}

bool VfxAsset::TryResolve(const char* keyOrBuiltin, GameWorld& world, VfxAsset& out) {
    if (keyOrBuiltin == nullptr || keyOrBuiltin[0] == '\0') {
        return false;
    }
    if (const VfxAsset* cached = world.TryGetVfxByKeyOrPath(keyOrBuiltin)) {
        out = *cached;
        return true;
    }
    const AssetLoadOutcome<VfxAsset> loaded = world.TryLoadVfx(keyOrBuiltin);
    if (loaded.ok) {
        if (const VfxAsset* cached = world.TryGetVfxByKeyOrPath(keyOrBuiltin)) {
            out = *cached;
            return true;
        }
        out = loaded.value;
        return true;
    }
    if (VfxLibrary::IsBuiltinName(keyOrBuiltin)) {
        out = FromBuiltin(keyOrBuiltin);
        return true;
    }
    if (IsCompositeBuiltin(keyOrBuiltin)) {
        out = FromBuiltin(keyOrBuiltin);
        return true;
    }
    return false;
}

VfxAsset VfxAsset::FromBuiltin(const char* builtinName, const std::uint32_t burst) {
    if (builtinName != nullptr && std::strcmp(builtinName, "fireworks") == 0) {
        return VfxEffectDefinition::Fireworks().ToAsset();
    }
    if (builtinName != nullptr && std::strcmp(builtinName, "confetti") == 0) {
        return VfxEffectDefinition::Confetti().ToAsset();
    }
    if (builtinName != nullptr && std::strcmp(builtinName, "meteor_strike") == 0) {
        return VfxEffectDefinition::MeteorStrike().ToAsset();
    }
    if (builtinName != nullptr && std::strcmp(builtinName, "magic_impact") == 0) {
        return VfxEffectDefinition::MagicImpact().ToAsset();
    }
    if (builtinName != nullptr && std::strcmp(builtinName, "smoke_grenade") == 0) {
        return VfxEffectDefinition::SmokeGrenade().ToAsset();
    }
    VfxAsset asset{};
    asset.builtinName = Utf8String(builtinName != nullptr ? builtinName : "");
    asset.burstCount = burst;
    if (burst == 0) {
        for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(VfxBuiltinId::Count); ++i) {
            const auto id = static_cast<VfxBuiltinId>(i);
            if (std::strcmp(asset.builtinName.CStr(), VfxLibrary::GetBuiltinName(id)) == 0) {
                asset.burstCount = VfxLibrary::GetDefaultBurstCount(id);
                break;
            }
        }
    }
    return asset;
}

}  // namespace Spark
