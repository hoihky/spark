#include "spark/scene/vfx/VfxEmitterParams.hpp"

#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"

namespace Spark {

void VfxEmitterParams::ApplyTo(ParticleEmitterComponent& emitter) const {
    emitter.SetEmitterEnabled(enabled);
    emitter.SetMaxParticles(maxParticles);
    emitter.SetEmissionRate(emissionRate);
    emitter.SetLifetime(lifeMin, lifeMax);
    emitter.SetStartEndSize(sizeStart, sizeEnd);
    emitter.SetStartEndColor(colorStart, colorEnd);
    emitter.SetGravity(gravity);
    emitter.SetEmissionDirection(emissionDir);
    emitter.SetUseLocalEmission(useLocalEmission);
    emitter.SetSpreadAngleRadians(spreadRadians);
    emitter.SetSpeedRange(speedMin, speedMax);
}

void VfxEmitterParams::CaptureFrom(const ParticleEmitterComponent& emitter) {
    enabled = emitter.IsEmitterEnabled();
    maxParticles = emitter.GetMaxParticles();
    emissionRate = emitter.GetEmissionRate();
    lifeMin = emitter.GetLifetimeMin();
    lifeMax = emitter.GetLifetimeMax();
    sizeStart = emitter.GetStartSize();
    sizeEnd = emitter.GetEndSize();
    colorStart = emitter.GetColorStart();
    colorEnd = emitter.GetColorEnd();
    gravity = emitter.GetGravity();
    emissionDir = emitter.GetEmissionDirection();
    useLocalEmission = emitter.GetUseLocalEmission();
    spreadRadians = emitter.GetSpreadAngleRadians();
    speedMin = emitter.GetSpeedMin();
    speedMax = emitter.GetSpeedMax();
}

}  // namespace Spark
