#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

class GameObject;
class ParticleEmitterComponent;

/** Per-frame emission strategy for <c>ParticleEmitterComponent</c> (continuous, burst-only, ring, …). */
class IParticleModule {
public:
    virtual ~IParticleModule() = default;

    [[nodiscard]] virtual const char* GetId() const noexcept = 0;

    virtual void EmitFrame(
            ParticleEmitterComponent& emitter,
            GameObject& owner,
            const Vector3& origin,
            const Vector3& worldEmissionDir,
            float deltaTimeSeconds) const = 0;
};

}  // namespace Spark
