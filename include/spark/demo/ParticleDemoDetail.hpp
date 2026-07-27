#pragma once

#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/math/Vector4.hpp"

namespace Spark {

namespace Detail {

constexpr int kParticleDemoEffectCount = 4;

inline void ApplyParticlePreset(const int presetIndex, ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    switch (presetIndex) {
    case 0:
        pe.SetMaxParticles(800);
        pe.SetEmissionRate(110.0F);
        pe.SetLifetime(0.22F, 0.55F);
        pe.SetStartEndSize(0.24F, 0.03F);
        pe.SetStartEndColor(Vector4{1.0F, 0.55F, 0.12F, 1.0F}, Vector4{0.85F, 0.05F, 0.0F, 0.0F});
        pe.SetGravity({0.0F, 0.35F, 0.0F});
        pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
        pe.SetSpreadAngleRadians(0.55F);
        pe.SetSpeedRange(1.6F, 4.2F);
        break;
    case 1:
        pe.SetMaxParticles(1400);
        pe.SetEmissionRate(240.0F);
        pe.SetLifetime(2.0F, 3.8F);
        pe.SetStartEndSize(0.07F, 0.035F);
        pe.SetStartEndColor(Vector4{0.95F, 0.97F, 1.0F, 0.95F}, Vector4{0.88F, 0.92F, 1.0F, 0.0F});
        pe.SetGravity({0.0F, -0.55F, 0.0F});
        pe.SetEmissionDirection(Vector3{0.05F, -1.0F, 0.02F}.Normalized());
        pe.SetSpreadAngleRadians(1.25F);
        pe.SetSpeedRange(0.15F, 1.1F);
        break;
    case 2:
        pe.SetMaxParticles(500);
        pe.SetEmissionRate(38.0F);
        pe.SetLifetime(1.4F, 2.6F);
        pe.SetStartEndSize(0.1F, 0.42F);
        pe.SetStartEndColor(Vector4{0.55F, 0.55F, 0.55F, 0.55F}, Vector4{0.35F, 0.35F, 0.35F, 0.0F});
        pe.SetGravity({0.0F, 0.85F, 0.0F});
        pe.SetEmissionDirection(Vector3{0.12F, 1.0F, 0.08F}.Normalized());
        pe.SetSpreadAngleRadians(0.95F);
        pe.SetSpeedRange(0.25F, 1.05F);
        break;
    default:
        pe.SetMaxParticles(900);
        pe.SetEmissionRate(85.0F);
        pe.SetLifetime(0.35F, 1.05F);
        pe.SetStartEndSize(0.11F, 0.02F);
        pe.SetStartEndColor(Vector4{0.35F, 0.95F, 1.0F, 1.0F}, Vector4{0.85F, 0.25F, 1.0F, 0.0F});
        pe.SetGravity({0.0F, 0.18F, 0.0F});
        pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
        pe.SetSpreadAngleRadians(2.05F);
        pe.SetSpeedRange(1.8F, 5.2F);
        break;
    }
    pe.SetMaxParticles(pe.GetMaxParticles());
}

}  // namespace Detail

}  // namespace Spark
