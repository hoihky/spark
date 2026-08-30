#include "spark/scene/vfx/VfxLibrary.hpp"

#include "spark/math/Vector4.hpp"
#include "spark/scene/vfx/ParticleCurves.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace Spark {

namespace {

bool StringsEqualIgnoreCase(const char* a, const char* b) noexcept {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    while (*a != '\0' && *b != '\0') {
        if (std::tolower(static_cast<unsigned char>(*a)) != std::tolower(static_cast<unsigned char>(*b))) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

void ApplySizeCurve(ParticleEmitterComponent& pe, const ParticleFloatCurve& curve) {
    pe.SetSizeCurve(curve);
}

void ApplyColorCurve(ParticleEmitterComponent& pe, const ParticleColorCurve& curve) {
    pe.SetColorCurve(curve);
}

ParticleFloatCurve SizeCurve3(const float t1, const float v1, const float t2, const float v2, const float end) {
    ParticleFloatCurve curve{};
    curve.keyframes[0] = {0.0F, v1};
    curve.keyframes[1] = {t1, v1};
    curve.keyframes[2] = {t2, v2};
    curve.keyframes[3] = {1.0F, end};
    curve.keyframeCount = 4;
    return curve;
}

ParticleColorCurve ColorCurve4(
        const Vector4& c0,
        const Vector4& c1,
        const Vector4& c2,
        const Vector4& c3) {
    ParticleColorCurve curve{};
    curve.keyframes[0] = {0.0F, c0};
    curve.keyframes[1] = {0.22F, c1};
    curve.keyframes[2] = {0.58F, c2};
    curve.keyframes[3] = {1.0F, c3};
    curve.keyframeCount = 4;
    return curve;
}

void ApplyFire(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(1200);
    pe.SetEmissionRate(145.0F);
    pe.SetLifetime(0.16F, 0.58F);
    pe.SetStartEndSize(0.3F, 0.02F);
    pe.SetStartEndColor(Vector4{1.0F, 0.92F, 0.45F, 1.0F}, Vector4{0.35F, 0.04F, 0.0F, 0.0F});
    pe.SetGravity({0.0F, 0.55F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.48F);
    pe.SetSpeedRange(1.8F, 4.8F);
    ApplySizeCurve(pe, SizeCurve3(0.18F, 0.34F, 0.55F, 0.16F, 0.02F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 0.95F, 0.55F, 1.0F},
                    {1.0F, 0.62F, 0.12F, 1.0F},
                    {0.92F, 0.22F, 0.03F, 0.65F},
                    {0.35F, 0.05F, 0.0F, 0.0F}));
}

void ApplySnow(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(1600);
    pe.SetEmissionRate(280.0F);
    pe.SetLifetime(2.2F, 4.2F);
    pe.SetStartEndSize(0.065F, 0.03F);
    pe.SetStartEndColor(Vector4{0.98F, 0.99F, 1.0F, 0.9F}, Vector4{0.88F, 0.92F, 1.0F, 0.0F});
    pe.SetGravity({0.0F, -0.48F, 0.0F});
    pe.SetEmissionDirection(Vector3{0.06F, -1.0F, 0.03F}.Normalized());
    pe.SetSpreadAngleRadians(1.35F);
    pe.SetSpeedRange(0.12F, 0.95F);
    ApplySizeCurve(pe, SizeCurve3(0.35F, 0.07F, 0.7F, 0.045F, 0.028F));
}

void ApplySmoke(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(720);
    pe.SetEmissionRate(52.0F);
    pe.SetLifetime(1.6F, 3.2F);
    pe.SetStartEndSize(0.08F, 0.52F);
    pe.SetStartEndColor(Vector4{0.62F, 0.62F, 0.62F, 0.45F}, Vector4{0.32F, 0.32F, 0.32F, 0.0F});
    pe.SetGravity({0.0F, 0.65F, 0.0F});
    pe.SetEmissionDirection(Vector3{0.1F, 1.0F, 0.06F}.Normalized());
    pe.SetSpreadAngleRadians(1.05F);
    pe.SetSpeedRange(0.2F, 1.15F);
    ApplySizeCurve(pe, SizeCurve3(0.2F, 0.12F, 0.55F, 0.38F, 0.5F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.55F, 0.55F, 0.55F, 0.35F},
                    {0.48F, 0.48F, 0.48F, 0.42F},
                    {0.38F, 0.38F, 0.38F, 0.18F},
                    {0.28F, 0.28F, 0.28F, 0.0F}));
}

void ApplySparkle(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(1000);
    pe.SetEmissionRate(105.0F);
    pe.SetLifetime(0.28F, 1.15F);
    pe.SetStartEndSize(0.12F, 0.015F);
    pe.SetStartEndColor(Vector4{0.45F, 0.98F, 1.0F, 1.0F}, Vector4{0.85F, 0.35F, 1.0F, 0.0F});
    pe.SetGravity({0.0F, 0.12F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(2.2F);
    pe.SetSpeedRange(2.0F, 5.8F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.55F, 1.0F, 1.0F, 1.0F},
                    {0.35F, 0.85F, 1.0F, 0.95F},
                    {0.75F, 0.45F, 1.0F, 0.55F},
                    {0.9F, 0.2F, 0.85F, 0.0F}));
}

void ApplyExplosion(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(320);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.1F, 0.48F);
    pe.SetStartEndSize(0.32F, 0.03F);
    pe.SetStartEndColor(Vector4{1.0F, 0.82F, 0.28F, 1.0F}, Vector4{0.45F, 0.06F, 0.02F, 0.0F});
    pe.SetGravity({0.0F, -2.8F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(2.45F);
    pe.SetSpeedRange(4.0F, 10.5F);
    ApplySizeCurve(pe, SizeCurve3(0.08F, 0.38F, 0.35F, 0.22F, 0.03F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 0.95F, 0.65F, 1.0F},
                    {1.0F, 0.55F, 0.12F, 1.0F},
                    {0.75F, 0.18F, 0.04F, 0.55F},
                    {0.25F, 0.05F, 0.02F, 0.0F}));
}

void ApplyImpact(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(128);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.06F, 0.24F);
    pe.SetStartEndSize(0.16F, 0.02F);
    pe.SetStartEndColor(Vector4{1.0F, 0.95F, 0.72F, 1.0F}, Vector4{0.72F, 0.32F, 0.08F, 0.0F});
    pe.SetGravity({0.0F, -5.2F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.72F);
    pe.SetSpeedRange(2.5F, 7.2F);
    ApplySizeCurve(pe, SizeCurve3(0.12F, 0.18F, 0.4F, 0.08F, 0.02F));
}

void ApplyRain(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(2200);
    pe.SetEmissionRate(380.0F);
    pe.SetLifetime(0.4F, 1.05F);
    pe.SetStartEndSize(0.022F, 0.016F);
    pe.SetStartEndColor(Vector4{0.58F, 0.68F, 0.88F, 0.7F}, Vector4{0.42F, 0.52F, 0.72F, 0.0F});
    pe.SetGravity({0.0F, -10.5F, 0.0F});
    pe.SetEmissionDirection(Vector3{0.06F, -1.0F, 0.03F}.Normalized());
    pe.SetSpreadAngleRadians(0.28F);
    pe.SetSpeedRange(7.0F, 12.5F);
}

void ApplyMuzzleFlash(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(80);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.02F, 0.08F);
    pe.SetStartEndSize(0.26F, 0.03F);
    pe.SetStartEndColor(Vector4{1.0F, 0.98F, 0.62F, 1.0F}, Vector4{1.0F, 0.42F, 0.04F, 0.0F});
    pe.SetGravity({0.0F, 0.0F, 0.0F});
    pe.SetEmissionDirection({0.0F, 0.0F, 1.0F});
    pe.SetSpreadAngleRadians(0.38F);
    pe.SetSpeedRange(5.0F, 10.5F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 1.0F, 0.75F, 1.0F},
                    {1.0F, 0.75F, 0.25F, 0.95F},
                    {1.0F, 0.45F, 0.05F, 0.45F},
                    {0.6F, 0.15F, 0.02F, 0.0F}));
}

void ApplyBlood(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(160);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.15F, 0.58F);
    pe.SetStartEndSize(0.1F, 0.018F);
    pe.SetStartEndColor(Vector4{0.82F, 0.06F, 0.05F, 1.0F}, Vector4{0.32F, 0.02F, 0.02F, 0.0F});
    pe.SetGravity({0.0F, -8.5F, 0.0F});
    pe.SetEmissionDirection({0.0F, 0.4F, 1.0F});
    pe.SetSpreadAngleRadians(1.15F);
    pe.SetSpeedRange(2.8F, 7.5F);
}

void ApplyHeal(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.62F);
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(520);
    pe.SetEmissionRate(88.0F);
    pe.SetLifetime(0.5F, 1.35F);
    pe.SetStartEndSize(0.09F, 0.018F);
    pe.SetStartEndColor(Vector4{0.42F, 1.0F, 0.52F, 1.0F}, Vector4{0.12F, 0.78F, 0.32F, 0.0F});
    pe.SetGravity({0.0F, 1.35F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.22F);
    pe.SetSpeedRange(0.55F, 1.95F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.65F, 1.0F, 0.75F, 1.0F},
                    {0.35F, 0.98F, 0.45F, 0.9F},
                    {0.2F, 0.85F, 0.35F, 0.45F},
                    {0.1F, 0.55F, 0.2F, 0.0F}));
}

void ApplyLevelUp(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(240);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.32F, 1.15F);
    pe.SetStartEndSize(0.18F, 0.025F);
    pe.SetStartEndColor(Vector4{1.0F, 0.94F, 0.28F, 1.0F}, Vector4{1.0F, 0.5F, 0.04F, 0.0F});
    pe.SetGravity({0.0F, -1.0F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(2.55F);
    pe.SetSpeedRange(2.2F, 7.0F);
    ApplySizeCurve(pe, SizeCurve3(0.15F, 0.2F, 0.45F, 0.12F, 0.02F));
}

void ApplyElectric(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(800);
    pe.SetEmissionRate(175.0F);
    pe.SetLifetime(0.04F, 0.16F);
    pe.SetStartEndSize(0.07F, 0.008F);
    pe.SetStartEndColor(Vector4{0.65F, 0.92F, 1.0F, 1.0F}, Vector4{0.22F, 0.42F, 1.0F, 0.0F});
    pe.SetGravity({0.0F, 0.0F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(2.75F);
    pe.SetSpeedRange(3.5F, 11.0F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.75F, 0.95F, 1.0F, 1.0F},
                    {0.45F, 0.75F, 1.0F, 0.95F},
                    {0.85F, 0.55F, 1.0F, 0.65F},
                    {0.25F, 0.35F, 0.95F, 0.0F}));
}

void ApplyPoison(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(640);
    pe.SetEmissionRate(62.0F);
    pe.SetLifetime(0.9F, 2.0F);
    pe.SetStartEndSize(0.1F, 0.36F);
    pe.SetStartEndColor(Vector4{0.38F, 0.98F, 0.28F, 0.6F}, Vector4{0.12F, 0.48F, 0.1F, 0.0F});
    pe.SetGravity({0.0F, 0.38F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.42F);
    pe.SetSpeedRange(0.3F, 1.05F);
    ApplySizeCurve(pe, SizeCurve3(0.25F, 0.14F, 0.6F, 0.28F, 0.34F));
}

void ApplyDust(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(96);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.22F, 0.72F);
    pe.SetStartEndSize(0.12F, 0.42F);
    pe.SetStartEndColor(Vector4{0.75F, 0.65F, 0.5F, 0.5F}, Vector4{0.52F, 0.45F, 0.35F, 0.0F});
    pe.SetGravity({0.0F, -0.28F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.22F);
    pe.SetSpeedRange(0.35F, 2.4F);
    ApplySizeCurve(pe, SizeCurve3(0.18F, 0.16F, 0.45F, 0.32F, 0.4F));
}

void ApplyDashTrail(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(420);
    pe.SetEmissionRate(220.0F);
    pe.SetLifetime(0.1F, 0.32F);
    pe.SetStartEndSize(0.11F, 0.015F);
    pe.SetStartEndColor(Vector4{0.5F, 0.88F, 1.0F, 0.88F}, Vector4{0.12F, 0.38F, 1.0F, 0.0F});
    pe.SetGravity({0.0F, 0.0F, 0.0F});
    pe.SetEmissionDirection({0.0F, 0.0F, -1.0F});
    pe.SetSpreadAngleRadians(0.28F);
    pe.SetSpeedRange(0.15F, 1.2F);
}

void ApplyAura(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.82F);
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(560);
    pe.SetEmissionRate(108.0F);
    pe.SetLifetime(0.42F, 1.05F);
    pe.SetStartEndSize(0.075F, 0.018F);
    pe.SetStartEndColor(Vector4{0.38F, 0.78F, 1.0F, 0.95F}, Vector4{0.12F, 0.42F, 1.0F, 0.0F});
    pe.SetGravity({0.0F, 0.42F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.16F);
    pe.SetSpeedRange(0.32F, 1.15F);
}

void ApplySoul(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(340);
    pe.SetEmissionRate(48.0F);
    pe.SetLifetime(0.72F, 1.55F);
    pe.SetStartEndSize(0.1F, 0.018F);
    pe.SetStartEndColor(Vector4{0.58F, 0.38F, 0.98F, 0.92F}, Vector4{0.12F, 0.04F, 0.32F, 0.0F});
    pe.SetGravity({0.0F, 1.2F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.92F);
    pe.SetSpeedRange(0.4F, 1.7F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.7F, 0.5F, 1.0F, 0.95F},
                    {0.55F, 0.32F, 0.92F, 0.75F},
                    {0.35F, 0.15F, 0.65F, 0.35F},
                    {0.12F, 0.05F, 0.28F, 0.0F}));
}

void ApplyLootSparkle(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.48F);
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(280);
    pe.SetEmissionRate(58.0F);
    pe.SetLifetime(0.32F, 0.92F);
    pe.SetStartEndSize(0.065F, 0.012F);
    pe.SetStartEndColor(Vector4{1.0F, 0.94F, 0.28F, 1.0F}, Vector4{1.0F, 0.52F, 0.04F, 0.0F});
    pe.SetGravity({0.0F, 0.32F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.2F);
    pe.SetSpeedRange(0.22F, 1.05F);
}

void ApplyWaterSplash(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(120);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.15F, 0.52F);
    pe.SetStartEndSize(0.09F, 0.018F);
    pe.SetStartEndColor(Vector4{0.48F, 0.78F, 1.0F, 0.95F}, Vector4{0.12F, 0.42F, 0.92F, 0.0F});
    pe.SetGravity({0.0F, -6.2F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.52F);
    pe.SetSpeedRange(2.0F, 6.2F);
}

void ApplyLeaves(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(580);
    pe.SetEmissionRate(42.0F);
    pe.SetLifetime(1.8F, 3.5F);
    pe.SetStartEndSize(0.095F, 0.048F);
    pe.SetStartEndColor(Vector4{0.48F, 0.75F, 0.2F, 0.95F}, Vector4{0.75F, 0.4F, 0.1F, 0.0F});
    pe.SetGravity({0.0F, -0.75F, 0.0F});
    pe.SetEmissionDirection(Vector3{-0.22F, -0.88F, 0.12F}.Normalized());
    pe.SetSpreadAngleRadians(1.12F);
    pe.SetSpeedRange(0.32F, 1.35F);
}

void ApplyMagicBolt(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(480);
    pe.SetEmissionRate(195.0F);
    pe.SetLifetime(0.06F, 0.2F);
    pe.SetStartEndSize(0.09F, 0.015F);
    pe.SetStartEndColor(Vector4{0.45F, 0.82F, 1.0F, 1.0F}, Vector4{0.15F, 0.35F, 0.95F, 0.0F});
    pe.SetGravity({0.0F, 0.0F, 0.0F});
    pe.SetEmissionDirection({0.0F, 0.0F, 1.0F});
    pe.SetSpreadAngleRadians(0.14F);
    pe.SetSpeedRange(7.5F, 14.0F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.55F, 0.9F, 1.0F, 1.0F},
                    {0.35F, 0.72F, 1.0F, 0.95F},
                    {0.65F, 0.45F, 1.0F, 0.7F},
                    {0.2F, 0.25F, 0.85F, 0.0F}));
}

void ApplyIceShatter(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(180);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.12F, 0.45F);
    pe.SetStartEndSize(0.11F, 0.02F);
    pe.SetStartEndColor(Vector4{0.85F, 0.95F, 1.0F, 1.0F}, Vector4{0.35F, 0.65F, 0.95F, 0.0F});
    pe.SetGravity({0.0F, -6.5F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(2.1F);
    pe.SetSpeedRange(3.0F, 8.5F);
    ApplySizeCurve(pe, SizeCurve3(0.1F, 0.14F, 0.35F, 0.08F, 0.02F));
}

void ApplyLaserHit(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(72);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.04F, 0.14F);
    pe.SetStartEndSize(0.14F, 0.02F);
    pe.SetStartEndColor(Vector4{0.35F, 1.0F, 0.65F, 1.0F}, Vector4{0.1F, 0.55F, 0.35F, 0.0F});
    pe.SetGravity({0.0F, -2.0F, 0.0F});
    pe.SetEmissionDirection({0.0F, 0.0F, 1.0F});
    pe.SetSpreadAngleRadians(0.85F);
    pe.SetSpeedRange(4.5F, 11.0F);
}

void ApplyEmbers(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(360);
    pe.SetEmissionRate(38.0F);
    pe.SetLifetime(0.85F, 2.2F);
    pe.SetStartEndSize(0.045F, 0.012F);
    pe.SetStartEndColor(Vector4{1.0F, 0.55F, 0.12F, 0.95F}, Vector4{0.45F, 0.08F, 0.02F, 0.0F});
    pe.SetGravity({0.0F, 0.85F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.45F);
    pe.SetSpeedRange(0.35F, 1.6F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 0.65F, 0.15F, 1.0F},
                    {0.95F, 0.35F, 0.05F, 0.85F},
                    {0.65F, 0.12F, 0.02F, 0.35F},
                    {0.25F, 0.05F, 0.01F, 0.0F}));
}

void ApplyHolyLight(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.7F);
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(500);
    pe.SetEmissionRate(92.0F);
    pe.SetLifetime(0.55F, 1.4F);
    pe.SetStartEndSize(0.085F, 0.016F);
    pe.SetStartEndColor(Vector4{1.0F, 0.98F, 0.72F, 1.0F}, Vector4{0.95F, 0.82F, 0.35F, 0.0F});
    pe.SetGravity({0.0F, 1.5F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.18F);
    pe.SetSpeedRange(0.5F, 1.75F);
}

void ApplyCurse(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(440);
    pe.SetEmissionRate(58.0F);
    pe.SetLifetime(0.95F, 2.1F);
    pe.SetStartEndSize(0.11F, 0.34F);
    pe.SetStartEndColor(Vector4{0.45F, 0.12F, 0.55F, 0.75F}, Vector4{0.15F, 0.45F, 0.12F, 0.0F});
    pe.SetGravity({0.0F, 0.55F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.55F);
    pe.SetSpeedRange(0.25F, 1.05F);
    ApplySizeCurve(pe, SizeCurve3(0.2F, 0.12F, 0.55F, 0.24F, 0.32F));
}

void ApplyRocketTrail(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(520);
    pe.SetEmissionRate(165.0F);
    pe.SetLifetime(0.18F, 0.55F);
    pe.SetStartEndSize(0.14F, 0.04F);
    pe.SetStartEndColor(Vector4{1.0F, 0.72F, 0.22F, 0.9F}, Vector4{0.45F, 0.42F, 0.42F, 0.0F});
    pe.SetGravity({0.0F, -0.15F, 0.0F});
    pe.SetEmissionDirection({0.0F, 0.0F, -1.0F});
    pe.SetSpreadAngleRadians(0.42F);
    pe.SetSpeedRange(0.5F, 2.8F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 0.78F, 0.25F, 0.95F},
                    {0.95F, 0.45F, 0.1F, 0.75F},
                    {0.55F, 0.35F, 0.32F, 0.35F},
                    {0.35F, 0.32F, 0.32F, 0.0F}));
}

void ApplyGroundFire(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(1.15F);
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(680);
    pe.SetEmissionRate(125.0F);
    pe.SetLifetime(0.22F, 0.62F);
    pe.SetStartEndSize(0.2F, 0.04F);
    pe.SetStartEndColor(Vector4{1.0F, 0.62F, 0.1F, 1.0F}, Vector4{0.55F, 0.05F, 0.0F, 0.0F});
    pe.SetGravity({0.0F, 0.75F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.35F);
    pe.SetSpeedRange(0.8F, 2.6F);
    ApplySizeCurve(pe, SizeCurve3(0.12F, 0.22F, 0.45F, 0.12F, 0.03F));
}

void ApplyShockwave(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(140);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.18F, 0.42F);
    pe.SetStartEndSize(0.08F, 0.48F);
    pe.SetStartEndColor(Vector4{0.92F, 0.92F, 0.95F, 0.55F}, Vector4{0.55F, 0.55F, 0.58F, 0.0F});
    pe.SetGravity({0.0F, 0.05F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.22F);
    pe.SetSpeedRange(4.5F, 9.0F);
    ApplySizeCurve(pe, SizeCurve3(0.08F, 0.1F, 0.35F, 0.32F, 0.45F));
}

void ApplyMeteorTrail(ParticleEmitterComponent& pe) {
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(420);
    pe.SetEmissionRate(135.0F);
    pe.SetLifetime(0.14F, 0.42F);
    pe.SetStartEndSize(0.16F, 0.035F);
    pe.SetStartEndColor(Vector4{1.0F, 0.55F, 0.12F, 1.0F}, Vector4{0.35F, 0.12F, 0.05F, 0.0F});
    pe.SetGravity({0.0F, -1.2F, 0.0F});
    pe.SetEmissionDirection({0.0F, -1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.32F);
    pe.SetSpeedRange(3.5F, 8.0F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 0.72F, 0.2F, 1.0F},
                    {1.0F, 0.42F, 0.05F, 0.9F},
                    {0.65F, 0.18F, 0.03F, 0.45F},
                    {0.25F, 0.08F, 0.02F, 0.0F}));
}

}  // namespace

void VfxLibrary::ApplyBuiltin(const VfxBuiltinId id, ParticleEmitterComponent& emitter) {
    switch (id) {
    case VfxBuiltinId::Fire:
        ApplyFire(emitter);
        break;
    case VfxBuiltinId::Snow:
        ApplySnow(emitter);
        break;
    case VfxBuiltinId::Smoke:
        ApplySmoke(emitter);
        break;
    case VfxBuiltinId::Sparkle:
        ApplySparkle(emitter);
        break;
    case VfxBuiltinId::Explosion:
        ApplyExplosion(emitter);
        break;
    case VfxBuiltinId::Impact:
        ApplyImpact(emitter);
        break;
    case VfxBuiltinId::Rain:
        ApplyRain(emitter);
        break;
    case VfxBuiltinId::MuzzleFlash:
        ApplyMuzzleFlash(emitter);
        break;
    case VfxBuiltinId::Blood:
        ApplyBlood(emitter);
        break;
    case VfxBuiltinId::Heal:
        ApplyHeal(emitter);
        break;
    case VfxBuiltinId::LevelUp:
        ApplyLevelUp(emitter);
        break;
    case VfxBuiltinId::Electric:
        ApplyElectric(emitter);
        break;
    case VfxBuiltinId::Poison:
        ApplyPoison(emitter);
        break;
    case VfxBuiltinId::Dust:
        ApplyDust(emitter);
        break;
    case VfxBuiltinId::DashTrail:
        ApplyDashTrail(emitter);
        break;
    case VfxBuiltinId::Aura:
        ApplyAura(emitter);
        break;
    case VfxBuiltinId::Soul:
        ApplySoul(emitter);
        break;
    case VfxBuiltinId::LootSparkle:
        ApplyLootSparkle(emitter);
        break;
    case VfxBuiltinId::WaterSplash:
        ApplyWaterSplash(emitter);
        break;
    case VfxBuiltinId::Leaves:
        ApplyLeaves(emitter);
        break;
    case VfxBuiltinId::MagicBolt:
        ApplyMagicBolt(emitter);
        break;
    case VfxBuiltinId::IceShatter:
        ApplyIceShatter(emitter);
        break;
    case VfxBuiltinId::LaserHit:
        ApplyLaserHit(emitter);
        break;
    case VfxBuiltinId::Embers:
        ApplyEmbers(emitter);
        break;
    case VfxBuiltinId::HolyLight:
        ApplyHolyLight(emitter);
        break;
    case VfxBuiltinId::Curse:
        ApplyCurse(emitter);
        break;
    case VfxBuiltinId::RocketTrail:
        ApplyRocketTrail(emitter);
        break;
    case VfxBuiltinId::GroundFire:
        ApplyGroundFire(emitter);
        break;
    case VfxBuiltinId::Shockwave:
        ApplyShockwave(emitter);
        break;
    case VfxBuiltinId::MeteorTrail:
        ApplyMeteorTrail(emitter);
        break;
    case VfxBuiltinId::Count:
        break;
    }
}

const char* VfxLibrary::GetBuiltinName(const VfxBuiltinId id) noexcept {
    switch (id) {
    case VfxBuiltinId::Fire:
        return "fire";
    case VfxBuiltinId::Snow:
        return "snow";
    case VfxBuiltinId::Smoke:
        return "smoke";
    case VfxBuiltinId::Sparkle:
        return "sparkle";
    case VfxBuiltinId::Explosion:
        return "explosion";
    case VfxBuiltinId::Impact:
        return "impact";
    case VfxBuiltinId::Rain:
        return "rain";
    case VfxBuiltinId::MuzzleFlash:
        return "muzzle_flash";
    case VfxBuiltinId::Blood:
        return "blood";
    case VfxBuiltinId::Heal:
        return "heal";
    case VfxBuiltinId::LevelUp:
        return "level_up";
    case VfxBuiltinId::Electric:
        return "electric";
    case VfxBuiltinId::Poison:
        return "poison";
    case VfxBuiltinId::Dust:
        return "dust";
    case VfxBuiltinId::DashTrail:
        return "dash_trail";
    case VfxBuiltinId::Aura:
        return "aura";
    case VfxBuiltinId::Soul:
        return "soul";
    case VfxBuiltinId::LootSparkle:
        return "loot_sparkle";
    case VfxBuiltinId::WaterSplash:
        return "water_splash";
    case VfxBuiltinId::Leaves:
        return "leaves";
    case VfxBuiltinId::MagicBolt:
        return "magic_bolt";
    case VfxBuiltinId::IceShatter:
        return "ice_shatter";
    case VfxBuiltinId::LaserHit:
        return "laser_hit";
    case VfxBuiltinId::Embers:
        return "embers";
    case VfxBuiltinId::HolyLight:
        return "holy_light";
    case VfxBuiltinId::Curse:
        return "curse";
    case VfxBuiltinId::RocketTrail:
        return "rocket_trail";
    case VfxBuiltinId::GroundFire:
        return "ground_fire";
    case VfxBuiltinId::Shockwave:
        return "shockwave";
    case VfxBuiltinId::MeteorTrail:
        return "meteor_trail";
    case VfxBuiltinId::Count:
        break;
    }
    return "";
}

std::uint32_t VfxLibrary::GetDefaultBurstCount(const VfxBuiltinId id) noexcept {
    switch (id) {
    case VfxBuiltinId::Explosion:
        return 112;
    case VfxBuiltinId::Impact:
        return 56;
    case VfxBuiltinId::MuzzleFlash:
        return 28;
    case VfxBuiltinId::Blood:
        return 42;
    case VfxBuiltinId::LevelUp:
        return 88;
    case VfxBuiltinId::Dust:
        return 24;
    case VfxBuiltinId::WaterSplash:
        return 48;
    case VfxBuiltinId::IceShatter:
        return 64;
    case VfxBuiltinId::LaserHit:
        return 36;
    case VfxBuiltinId::Shockwave:
        return 72;
    default:
        return 0;
    }
}

bool VfxLibrary::TryApplyBuiltinByName(const char* name, ParticleEmitterComponent& emitter) {
    if (name == nullptr || name[0] == '\0') {
        return false;
    }
    for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(VfxBuiltinId::Count); ++i) {
        const auto id = static_cast<VfxBuiltinId>(i);
        if (StringsEqualIgnoreCase(name, GetBuiltinName(id))) {
            ApplyBuiltin(id, emitter);
            return true;
        }
    }
    return false;
}

bool VfxLibrary::IsBuiltinName(const char* name) noexcept {
    if (name == nullptr || name[0] == '\0') {
        return false;
    }
    for (std::uint8_t i = 0; i < static_cast<std::uint8_t>(VfxBuiltinId::Count); ++i) {
        if (StringsEqualIgnoreCase(name, GetBuiltinName(static_cast<VfxBuiltinId>(i)))) {
            return true;
        }
    }
    return false;
}

}  // namespace Spark
