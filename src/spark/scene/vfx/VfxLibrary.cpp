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
    pe.SetRenderSpace(ParticleRenderSpace::SpriteLayer);
    pe.SetSpriteLayerSortOrder(420);
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
    pe.SetMaxParticles(96);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.04F, 0.16F);
    pe.SetStartEndSize(0.12F, 0.015F);
    pe.SetStartEndColor(Vector4{1.0F, 0.98F, 0.88F, 1.0F}, Vector4{0.85F, 0.42F, 0.12F, 0.0F});
    pe.SetGravity({0.0F, -6.5F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.38F);
    pe.SetSpeedRange(3.5F, 8.5F);
    ApplySizeCurve(pe, SizeCurve3(0.08F, 0.16F, 0.32F, 0.06F, 0.01F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 1.0F, 0.92F, 1.0F},
                    {1.0F, 0.82F, 0.35F, 0.95F},
                    {0.92F, 0.45F, 0.1F, 0.5F},
                    {0.45F, 0.12F, 0.04F, 0.0F}));
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
    pe.SetRenderSpace(ParticleRenderSpace::SpriteLayer);
    pe.SetSpriteLayerSortOrder(380);
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
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.12F);
    pe.SetUseLocalEmission(true);
    pe.SetMaxParticles(160);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.14F, 0.38F);
    pe.SetStartEndSize(0.06F, 0.52F);
    pe.SetStartEndColor(Vector4{0.95F, 0.96F, 1.0F, 0.62F}, Vector4{0.62F, 0.64F, 0.7F, 0.0F});
    pe.SetGravity({0.0F, 0.02F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(0.12F);
    pe.SetSpeedRange(5.5F, 11.0F);
    ApplySizeCurve(pe, SizeCurve3(0.06F, 0.08F, 0.28F, 0.38F, 0.48F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 1.0F, 1.0F, 0.7F},
                    {0.88F, 0.9F, 0.95F, 0.55F},
                    {0.72F, 0.74F, 0.78F, 0.28F},
                    {0.55F, 0.56F, 0.6F, 0.0F}));
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

void BeginSpriteLayer2D(ParticleEmitterComponent& pe, const std::int32_t sortOrder) {
    pe.SetRenderSpace(ParticleRenderSpace::SpriteLayer);
    pe.SetSpriteLayerSortOrder(sortOrder);
    pe.SetUseLocalEmission(false);
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
}

void ApplyHitSpark2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 460);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetMaxParticles(64);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.05F, 0.14F);
    pe.SetStartEndSize(0.04F, 0.14F);
    pe.SetStartEndColor(Vector4{1.0F, 0.98F, 0.92F, 1.0F}, Vector4{1.0F, 0.42F, 0.08F, 0.0F});
    pe.SetGravity({0.0F, -1.2F, 0.0F});
    pe.SetSpreadAngleRadians(2.65F);
    pe.SetSpeedRange(4.5F, 9.5F);
    ApplySizeCurve(pe, SizeCurve3(0.05F, 0.12F, 0.25F, 0.06F, 0.01F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 1.0F, 0.95F, 1.0F},
                    {1.0F, 0.75F, 0.25F, 1.0F},
                    {1.0F, 0.35F, 0.05F, 0.55F},
                    {0.45F, 0.08F, 0.02F, 0.0F}));
}

void ApplyCoinPop2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 440);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetMaxParticles(42);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.18F, 0.48F);
    pe.SetStartEndSize(0.05F, 0.22F);
    pe.SetStartEndColor(Vector4{1.0F, 0.95F, 0.22F, 1.0F}, Vector4{1.0F, 0.62F, 0.05F, 0.0F});
    pe.SetGravity({0.0F, -2.4F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.05F);
    pe.SetSpeedRange(2.2F, 5.5F);
    ApplySizeCurve(pe, SizeCurve3(0.12F, 0.08F, 0.45F, 0.18F, 0.04F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 0.98F, 0.45F, 1.0F},
                    {1.0F, 0.82F, 0.15F, 1.0F},
                    {0.95F, 0.55F, 0.05F, 0.65F},
                    {0.55F, 0.28F, 0.02F, 0.0F}));
}

void ApplyJumpRing2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 350);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.38F);
    pe.SetMaxParticles(80);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.12F, 0.26F);
    pe.SetStartEndSize(0.06F, 0.2F);
    pe.SetStartEndColor(Vector4{0.55F, 0.92F, 1.0F, 0.95F}, Vector4{0.2F, 0.45F, 0.95F, 0.0F});
    pe.SetGravity({0.0F, 0.35F, 0.0F});
    pe.SetSpeedRange(1.4F, 3.2F);
    ApplySizeCurve(pe, SizeCurve3(0.1F, 0.1F, 0.4F, 0.16F, 0.02F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.65F, 0.95F, 1.0F, 0.95F},
                    {0.45F, 0.82F, 1.0F, 0.85F},
                    {0.25F, 0.55F, 0.98F, 0.45F},
                    {0.12F, 0.35F, 0.85F, 0.0F}));
}

void ApplyLanternGlow2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 410);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetMaxParticles(160);
    pe.SetEmissionRate(32.0F);
    pe.SetLifetime(0.45F, 1.25F);
    pe.SetStartEndSize(0.14F, 0.02F);
    pe.SetStartEndColor(Vector4{1.0F, 0.78F, 0.22F, 0.82F}, Vector4{0.85F, 0.18F, 0.02F, 0.0F});
    pe.SetGravity({0.0F, 0.55F, 0.0F});
    pe.SetSpreadAngleRadians(0.55F);
    pe.SetSpeedRange(0.15F, 0.65F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 0.85F, 0.35F, 0.9F},
                    {1.0F, 0.55F, 0.12F, 0.75F},
                    {0.85F, 0.25F, 0.05F, 0.35F},
                    {0.35F, 0.08F, 0.02F, 0.0F}));
}

void ApplyRainSplash2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 320);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetMaxParticles(56);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.05F, 0.14F);
    pe.SetStartEndSize(0.03F, 0.1F);
    pe.SetStartEndColor(Vector4{0.72F, 0.88F, 1.0F, 0.75F}, Vector4{0.42F, 0.58F, 0.82F, 0.0F});
    pe.SetGravity({0.0F, -0.85F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.35F);
    pe.SetSpeedRange(1.2F, 3.5F);
    ApplySizeCurve(pe, SizeCurve3(0.08F, 0.05F, 0.35F, 0.09F, 0.02F));
}

void ApplySlashArc2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 470);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetMaxParticles(36);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.06F, 0.16F);
    pe.SetStartEndSize(0.14F, 0.38F);
    pe.SetStartEndColor(Vector4{0.92F, 0.98F, 1.0F, 0.95F}, Vector4{0.35F, 0.65F, 1.0F, 0.0F});
    pe.SetGravity({0.0F, -0.15F, 0.0F});
    pe.SetEmissionDirection({1.0F, 0.08F, 0.0F});
    pe.SetSpreadAngleRadians(0.28F);
    pe.SetSpeedRange(5.5F, 11.0F);
    ApplySizeCurve(pe, SizeCurve3(0.05F, 0.22F, 0.35F, 0.28F, 0.04F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.85F, 0.98F, 1.0F, 1.0F},
                    {0.55F, 0.82F, 1.0F, 0.9F},
                    {0.35F, 0.55F, 0.98F, 0.45F},
                    {0.15F, 0.35F, 0.75F, 0.0F}));
}

void ApplyFootstepPuff2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 300);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetMaxParticles(28);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.22F, 0.55F);
    pe.SetStartEndSize(0.08F, 0.38F);
    pe.SetStartEndColor(Vector4{0.78F, 0.72F, 0.62F, 0.42F}, Vector4{0.48F, 0.42F, 0.36F, 0.0F});
    pe.SetGravity({0.0F, 0.12F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.55F);
    pe.SetSpeedRange(0.18F, 0.72F);
    ApplySizeCurve(pe, SizeCurve3(0.15F, 0.1F, 0.5F, 0.32F, 0.38F));
}

void ApplyBlockImpact2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 455);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetMaxParticles(40);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.12F, 0.32F);
    pe.SetStartEndSize(0.06F, 0.16F);
    pe.SetStartEndColor(Vector4{0.95F, 0.96F, 1.0F, 1.0F}, Vector4{0.55F, 0.58F, 0.68F, 0.0F});
    pe.SetGravity({0.0F, -2.8F, 0.0F});
    pe.SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe.SetSpreadAngleRadians(1.15F);
    pe.SetSpeedRange(2.8F, 6.5F);
    ApplySizeCurve(pe, SizeCurve3(0.1F, 0.1F, 0.4F, 0.08F, 0.02F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 1.0F, 1.0F, 1.0F},
                    {0.85F, 0.88F, 0.95F, 0.85F},
                    {0.62F, 0.65F, 0.72F, 0.45F},
                    {0.38F, 0.4F, 0.48F, 0.0F}));
}

void ApplyMagicNova2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 480);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.2F);
    pe.SetMaxParticles(120);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.2F, 0.45F);
    pe.SetStartEndSize(0.1F, 0.42F);
    pe.SetStartEndColor(Vector4{0.72F, 0.28F, 1.0F, 1.0F}, Vector4{0.18F, 0.72F, 1.0F, 0.0F});
    pe.SetGravity({0.0F, 0.05F, 0.0F});
    pe.SetSpeedRange(2.5F, 5.5F);
    ApplySizeCurve(pe, SizeCurve3(0.08F, 0.14F, 0.4F, 0.32F, 0.04F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.85F, 0.45F, 1.0F, 1.0F},
                    {0.55F, 0.25F, 0.98F, 0.9F},
                    {0.35F, 0.65F, 1.0F, 0.55F},
                    {0.12F, 0.35F, 0.85F, 0.0F}));
}

void ApplyHealSparkle2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 430);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("burst_only");
    pe.SetMaxParticles(56);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.28F, 0.72F);
    pe.SetStartEndSize(0.05F, 0.14F);
    pe.SetStartEndColor(Vector4{0.35F, 1.0F, 0.48F, 1.0F}, Vector4{0.95F, 1.0F, 0.65F, 0.0F});
    pe.SetGravity({0.0F, 1.35F, 0.0F});
    pe.SetSpreadAngleRadians(2.1F);
    pe.SetSpeedRange(1.2F, 3.2F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.55F, 1.0F, 0.72F, 1.0F},
                    {0.35F, 0.98F, 0.42F, 0.95F},
                    {0.75F, 1.0F, 0.55F, 0.55F},
                    {0.25F, 0.75F, 0.35F, 0.0F}));
}

void ApplyPoisonBubble2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 390);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetMaxParticles(72);
    pe.SetEmissionRate(14.0F);
    pe.SetLifetime(0.55F, 1.35F);
    pe.SetStartEndSize(0.14F, 0.32F);
    pe.SetStartEndColor(Vector4{0.45F, 0.98F, 0.22F, 0.62F}, Vector4{0.55F, 0.15F, 0.72F, 0.0F});
    pe.SetGravity({0.0F, 0.65F, 0.0F});
    pe.SetSpreadAngleRadians(0.42F);
    pe.SetSpeedRange(0.22F, 0.75F);
    ApplySizeCurve(pe, SizeCurve3(0.2F, 0.14F, 0.55F, 0.26F, 0.3F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.55F, 0.98F, 0.35F, 0.65F},
                    {0.35F, 0.85F, 0.22F, 0.55F},
                    {0.62F, 0.25F, 0.78F, 0.35F},
                    {0.22F, 0.12F, 0.42F, 0.0F}));
}

void ApplyShieldPulse2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 475);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.48F);
    pe.SetMaxParticles(96);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.16F, 0.34F);
    pe.SetStartEndSize(0.04F, 0.22F);
    pe.SetStartEndColor(Vector4{0.45F, 0.88F, 1.0F, 0.9F}, Vector4{0.12F, 0.42F, 0.95F, 0.0F});
    pe.SetGravity({0.0F, 0.0F, 0.0F});
    pe.SetSpeedRange(1.8F, 4.2F);
    ApplySizeCurve(pe, SizeCurve3(0.1F, 0.06F, 0.45F, 0.18F, 0.03F));
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {0.55F, 0.92F, 1.0F, 0.95F},
                    {0.35F, 0.72F, 1.0F, 0.75F},
                    {0.22F, 0.52F, 0.98F, 0.4F},
                    {0.1F, 0.32F, 0.82F, 0.0F}));
}

void ApplyWaterRipple2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 310);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("ring");
    pe.SetRingRadius(0.08F);
    pe.SetMaxParticles(64);
    pe.SetEmissionRate(0.0F);
    pe.SetLifetime(0.28F, 0.58F);
    pe.SetStartEndSize(0.03F, 0.32F);
    pe.SetStartEndColor(Vector4{0.68F, 0.88F, 1.0F, 0.5F}, Vector4{0.38F, 0.62F, 0.92F, 0.0F});
    pe.SetGravity({0.0F, 0.0F, 0.0F});
    pe.SetSpeedRange(0.55F, 1.35F);
    ApplySizeCurve(pe, SizeCurve3(0.12F, 0.05F, 0.5F, 0.28F, 0.35F));
}

void ApplyEmberMotif2D(ParticleEmitterComponent& pe) {
    BeginSpriteLayer2D(pe, 400);
    pe.SetEmitterEnabled(true);
    pe.SetEmissionModuleId("continuous");
    pe.SetMaxParticles(180);
    pe.SetEmissionRate(36.0F);
    pe.SetLifetime(0.4F, 1.35F);
    pe.SetStartEndSize(0.06F, 0.012F);
    pe.SetStartEndColor(Vector4{1.0F, 0.62F, 0.15F, 0.95F}, Vector4{0.35F, 0.05F, 0.01F, 0.0F});
    pe.SetGravity({0.0F, 0.75F, 0.0F});
    pe.SetSpreadAngleRadians(0.72F);
    pe.SetSpeedRange(0.28F, 1.05F);
    ApplyColorCurve(
            pe,
            ColorCurve4(
                    {1.0F, 0.72F, 0.22F, 1.0F},
                    {0.98F, 0.42F, 0.08F, 0.85F},
                    {0.72F, 0.18F, 0.03F, 0.4F},
                    {0.28F, 0.05F, 0.01F, 0.0F}));
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
    case VfxBuiltinId::HitSpark2D:
        ApplyHitSpark2D(emitter);
        break;
    case VfxBuiltinId::CoinPop2D:
        ApplyCoinPop2D(emitter);
        break;
    case VfxBuiltinId::JumpRing2D:
        ApplyJumpRing2D(emitter);
        break;
    case VfxBuiltinId::LanternGlow2D:
        ApplyLanternGlow2D(emitter);
        break;
    case VfxBuiltinId::RainSplash2D:
        ApplyRainSplash2D(emitter);
        break;
    case VfxBuiltinId::SlashArc2D:
        ApplySlashArc2D(emitter);
        break;
    case VfxBuiltinId::FootstepPuff2D:
        ApplyFootstepPuff2D(emitter);
        break;
    case VfxBuiltinId::BlockImpact2D:
        ApplyBlockImpact2D(emitter);
        break;
    case VfxBuiltinId::MagicNova2D:
        ApplyMagicNova2D(emitter);
        break;
    case VfxBuiltinId::HealSparkle2D:
        ApplyHealSparkle2D(emitter);
        break;
    case VfxBuiltinId::PoisonBubble2D:
        ApplyPoisonBubble2D(emitter);
        break;
    case VfxBuiltinId::ShieldPulse2D:
        ApplyShieldPulse2D(emitter);
        break;
    case VfxBuiltinId::WaterRipple2D:
        ApplyWaterRipple2D(emitter);
        break;
    case VfxBuiltinId::EmberMotif2D:
        ApplyEmberMotif2D(emitter);
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
    case VfxBuiltinId::HitSpark2D:
        return "hit_spark_2d";
    case VfxBuiltinId::CoinPop2D:
        return "coin_pop_2d";
    case VfxBuiltinId::JumpRing2D:
        return "jump_ring_2d";
    case VfxBuiltinId::LanternGlow2D:
        return "lantern_glow_2d";
    case VfxBuiltinId::RainSplash2D:
        return "rain_splash_2d";
    case VfxBuiltinId::SlashArc2D:
        return "slash_arc_2d";
    case VfxBuiltinId::FootstepPuff2D:
        return "footstep_puff_2d";
    case VfxBuiltinId::BlockImpact2D:
        return "block_impact_2d";
    case VfxBuiltinId::MagicNova2D:
        return "magic_nova_2d";
    case VfxBuiltinId::HealSparkle2D:
        return "heal_sparkle_2d";
    case VfxBuiltinId::PoisonBubble2D:
        return "poison_bubble_2d";
    case VfxBuiltinId::ShieldPulse2D:
        return "shield_pulse_2d";
    case VfxBuiltinId::WaterRipple2D:
        return "water_ripple_2d";
    case VfxBuiltinId::EmberMotif2D:
        return "ember_motif_2d";
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
    case VfxBuiltinId::Sparkle:
        return 48;
    case VfxBuiltinId::LootSparkle:
        return 36;
    case VfxBuiltinId::Heal:
        return 64;
    case VfxBuiltinId::Electric:
        return 40;
    case VfxBuiltinId::MagicBolt:
        return 32;
    case VfxBuiltinId::Embers:
        return 28;
    case VfxBuiltinId::HolyLight:
        return 56;
    case VfxBuiltinId::Curse:
        return 48;
    case VfxBuiltinId::HitSpark2D:
        return 18;
    case VfxBuiltinId::CoinPop2D:
        return 14;
    case VfxBuiltinId::JumpRing2D:
        return 48;
    case VfxBuiltinId::RainSplash2D:
        return 12;
    case VfxBuiltinId::SlashArc2D:
        return 16;
    case VfxBuiltinId::FootstepPuff2D:
        return 10;
    case VfxBuiltinId::BlockImpact2D:
        return 20;
    case VfxBuiltinId::MagicNova2D:
        return 72;
    case VfxBuiltinId::HealSparkle2D:
        return 32;
    case VfxBuiltinId::ShieldPulse2D:
        return 64;
    case VfxBuiltinId::WaterRipple2D:
        return 40;
    case VfxBuiltinId::LanternGlow2D:
        return 26;
    case VfxBuiltinId::PoisonBubble2D:
        return 16;
    case VfxBuiltinId::EmberMotif2D:
        return 32;
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
