#pragma once

#include "spark/scene/vfx/modules/IParticleModule.hpp"

namespace Spark {

class GameObject;
class ParticleEmitterComponent;

/** Built-in emission modules registered by id (<c>continuous</c>, <c>burst_only</c>, <c>ring</c>). */
class ParticleModuleRegistry final {
public:
    [[nodiscard]] static const IParticleModule& Get(const char* moduleId) noexcept;
    [[nodiscard]] static const char* DefaultModuleId() noexcept { return "continuous"; }

    static void EmitFrame(
            ParticleEmitterComponent& emitter,
            GameObject& owner,
            const Vector3& origin,
            const Vector3& worldEmissionDir,
            float deltaTimeSeconds) noexcept;
};

}  // namespace Spark
