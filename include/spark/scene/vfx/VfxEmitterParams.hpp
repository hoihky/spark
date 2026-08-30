#pragma once

#include "spark/math/Vector3.hpp"
#include "spark/math/Vector4.hpp"

#include <cstdint>

namespace Spark {

class ParticleEmitterComponent;

/** Serializable particle emitter tuning (shared by <c>VfxAsset</c> and custom <c>.sparkvfx</c> files). */
struct VfxEmitterParams {
    bool enabled = true;
    std::uint32_t maxParticles = 512;
    float emissionRate = 48.0F;
    float lifeMin = 0.8F;
    float lifeMax = 1.6F;
    float sizeStart = 0.14F;
    float sizeEnd = 0.02F;
    Vector4 colorStart{0.95F, 0.85F, 0.35F, 1.0F};
    Vector4 colorEnd{0.9F, 0.2F, 0.05F, 0.0F};
    Vector3 gravity{0.0F, -1.8F, 0.0F};
    Vector3 emissionDir{0.0F, 1.0F, 0.0F};
    bool useLocalEmission = false;
    float spreadRadians = 0.55F;
    float speedMin = 1.2F;
    float speedMax = 2.8F;

    void ApplyTo(ParticleEmitterComponent& emitter) const;
    void CaptureFrom(const ParticleEmitterComponent& emitter);
};

}  // namespace Spark
