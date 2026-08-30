#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/vfx/VfxEffectDefinition.hpp"
#include "spark/scene/vfx/VfxEmitterParams.hpp"

#include <cstdint>

namespace Spark {

class GameObject;
class GameWorld;
class ParticleEmitterComponent;

/**
 * Shared VFX definition (built-in reference, custom emitter, or composite layers).
 * Loaded from <c>.sparkvfx</c> or synthesized from a built-in name.
 */
class VfxAsset {
public:
    Utf8String name;
    /** Single-emitter built-in (ignored when composite). */
    Utf8String builtinName;
    VfxEmitterParams emitter{};
    std::uint32_t burstCount = 0;
    VfxEffectDefinition definition{};

    [[nodiscard]] bool IsComposite() const noexcept { return definition.IsComposite(); }
    [[nodiscard]] const Utf8String& GetPrefabScenePath() const noexcept { return definition.prefabScenePath; }

    void ApplyTo(ParticleEmitterComponent& emitterOut) const;
    void Activate(ParticleEmitterComponent& emitterOut, GameObject& owner, VfxPlaybackMode mode) const;

    /** Resolves a cache key, disk path, or built-in name (e.g. <c>explosion</c>). */
    [[nodiscard]] static bool TryResolve(const char* keyOrBuiltin, GameWorld& world, VfxAsset& out);

    [[nodiscard]] static VfxAsset FromBuiltin(const char* builtinName, std::uint32_t burst = 0);
    [[nodiscard]] static bool IsCompositeBuiltin(const char* builtinName) noexcept;
};

}  // namespace Spark
