#include "spark/scene/water/GerstnerWaveSurface.hpp"

#include <cmath>

namespace Spark {

namespace {

constexpr float kTwoPi = 6.283185307F;

}  // namespace

GerstnerWaveSurface::GerstnerWaveSurface(WaterWaveSettings settingsIn) noexcept : settings(settingsIn) {}

GerstnerWaveSurface GerstnerWaveSurface::FromPreset(const WaterWavePresetId preset, const float globalWindSpeed) noexcept {
    return GerstnerWaveSurface(WaterWaveSettings::FromPreset(preset, globalWindSpeed));
}

void GerstnerWaveSurface::AccumulateWave(
        const GerstnerWave& wave,
        const float worldX,
        const float worldZ,
        const float timeSeconds,
        Accumulation& out) const noexcept {
    if (wave.amplitude <= 0.0F || wave.wavelength <= 1.0e-3F) {
        return;
    }

    float dirX = wave.direction.x;
    float dirZ = wave.direction.y;
    const float dirLen = std::sqrt(dirX * dirX + dirZ * dirZ);
    if (dirLen > 1.0e-6F) {
        dirX /= dirLen;
        dirZ /= dirLen;
    } else {
        dirX = 1.0F;
        dirZ = 0.0F;
    }

    const float k = kTwoPi / wave.wavelength;
    const float phase = k * (dirX * worldX + dirZ * worldZ) - wave.speed * timeSeconds;
    const float s = std::sin(phase);
    const float c = std::cos(phase);
    const float Q = wave.steepness;
    const float A = wave.amplitude;
    const float wA = A * k;

    out.horizontalDisp.x += Q * A * dirX * s;
    out.horizontalDisp.y += Q * A * dirZ * s;
    out.height += A * c;

    out.tangent.x += 1.0F - Q * wA * dirX * dirX * c;
    out.tangent.y += -wA * dirX * s;
    out.tangent.z += -Q * wA * dirX * dirZ * c;

    out.binormal.x += -Q * wA * dirX * dirZ * c;
    out.binormal.y += -wA * dirZ * s;
    out.binormal.z += 1.0F - Q * wA * dirZ * dirZ * c;
}

GerstnerWaveSurface::Accumulation GerstnerWaveSurface::Evaluate(
        const float worldX,
        const float worldZ,
        const float timeSeconds) const noexcept {
    Accumulation out{};
    for (std::uint32_t wi = 0; wi < settings.GetActiveWaveCount(); ++wi) {
        AccumulateWave(settings.GetWave(wi), worldX, worldZ, timeSeconds, out);
    }
    return out;
}

float GerstnerWaveSurface::SampleHeight(
        const float worldX,
        const float worldZ,
        const float timeSeconds) const noexcept {
    return Evaluate(worldX, worldZ, timeSeconds).height;
}

Vector3 GerstnerWaveSurface::SampleNormal(
        const float worldX,
        const float worldZ,
        const float timeSeconds) const noexcept {
    const Accumulation out = Evaluate(worldX, worldZ, timeSeconds);
    return Vector3::Cross(out.binormal, out.tangent).Normalized();
}

Vector2 GerstnerWaveSurface::SampleHorizontalDisplacement(
        const float worldX,
        const float worldZ,
        const float timeSeconds) const noexcept {
    return Evaluate(worldX, worldZ, timeSeconds).horizontalDisp;
}

}  // namespace Spark
