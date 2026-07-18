#include "spark/render/lighting/SceneLightingProfile.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

[[nodiscard]] float PickOverride(const float overrideValue, const float presetValue) noexcept {
    return (overrideValue > 0.0F) ? overrideValue : presetValue;
}

[[nodiscard]] float PickFadeStartRatio(const float overrideValue, const float presetValue) noexcept {
    return (overrideValue > 0.0F) ? overrideValue : presetValue;
}

[[nodiscard]] float Saturate(const float x) noexcept {
    return std::clamp(x, 0.0F, 1.0F);
}

/** Sun elevation 0..1 from normalized day phase (0 = midnight). */
[[nodiscard]] float SunElevation01(const float t) noexcept {
    const float angle = (t - 0.25F) * 6.28318530718F;
    return Saturate(std::sin(angle));
}

[[nodiscard]] Vector3 Lerp3(const Vector3& a, const Vector3& b, const float w) noexcept {
    return a + (b - a) * w;
}

}  // namespace

SceneLightingProfileSettings LightingProfileSettingsFor(const SceneLightingProfile profile) noexcept {
    SceneLightingProfileSettings s{};
    switch (profile) {
    case SceneLightingProfile::Outdoor:
        s.exposure = 0.72F;
        s.shadowCascadeNear = 0.12F;
        s.shadowCascadeFar = 400.0F;
        s.ambientScale = 0.9F;
        s.shadow.directionalEnabled = true;
        s.shadow.distanceFadeEnd = 150.0F;
        s.shadow.distanceFadeStartRatio = 0.82F;
        s.shadow.cascadeBlendFraction = 0.12F;
        s.shadow.defaultCast = true;
        s.shadow.defaultReceive = true;
        s.ambient.groundColor = {0.07F, 0.075F, 0.06F};
        s.ambient.skyColor = {0.14F, 0.17F, 0.22F};
        s.ambient.probeColor = {0.05F, 0.06F, 0.08F};
        s.ambient.probeWeight = 0.22F;
        s.timeOfDay.enabled = true;
        s.timeOfDay.normalizedTime = 0.42F;
        break;
    case SceneLightingProfile::Interior:
        s.exposure = 0.78F;
        s.shadowCascadeNear = 0.12F;
        s.shadowCascadeFar = 75.0F;
        s.ambientScale = 0.98F;
        s.shadow.directionalEnabled = true;
        s.shadow.distanceFadeEnd = 45.0F;
        s.shadow.distanceFadeStartRatio = 0.78F;
        s.shadow.cascadeBlendFraction = 0.15F;
        s.shadow.defaultCast = true;
        s.shadow.defaultReceive = true;
        s.ambient.groundColor = {0.09F, 0.085F, 0.08F};
        s.ambient.skyColor = {0.11F, 0.12F, 0.15F};
        s.ambient.probeColor = {0.08F, 0.075F, 0.07F};
        s.ambient.probeWeight = 0.48F;
        s.timeOfDay.enabled = false;
        s.timeOfDay.normalizedTime = 0.5F;
        break;
    case SceneLightingProfile::NightInterior:
        s.exposure = 0.62F;
        s.shadowCascadeNear = 0.12F;
        s.shadowCascadeFar = 60.0F;
        s.ambientScale = 1.05F;
        s.shadow.directionalEnabled = true;
        s.shadow.distanceFadeEnd = 38.0F;
        s.shadow.distanceFadeStartRatio = 0.75F;
        s.shadow.cascadeBlendFraction = 0.10F;
        s.shadow.defaultCast = true;
        s.shadow.defaultReceive = true;
        s.ambient.groundColor = {0.035F, 0.038F, 0.05F};
        s.ambient.skyColor = {0.05F, 0.06F, 0.09F};
        s.ambient.probeColor = {0.09F, 0.085F, 0.12F};
        s.ambient.probeWeight = 0.72F;
        s.timeOfDay.enabled = true;
        s.timeOfDay.normalizedTime = 0.04F;
        break;
    case SceneLightingProfile::Default:
    default:
        s.exposure = 0.68F;
        s.shadowCascadeNear = 0.12F;
        s.shadowCascadeFar = 400.0F;
        s.ambientScale = 0.88F;
        s.shadow.directionalEnabled = true;
        s.shadow.distanceFadeEnd = 0.0F;
        s.shadow.distanceFadeStartRatio = 0.82F;
        s.shadow.cascadeBlendFraction = 0.12F;
        s.shadow.defaultCast = true;
        s.shadow.defaultReceive = true;
        s.ambient.groundColor = {0.05F, 0.055F, 0.07F};
        s.ambient.skyColor = {0.11F, 0.13F, 0.17F};
        s.ambient.probeColor = {0.04F, 0.045F, 0.06F};
        s.ambient.probeWeight = 0.3F;
        s.timeOfDay.enabled = false;
        s.timeOfDay.normalizedTime = 0.5F;
        break;
    }
    return s;
}

std::int32_t DefaultShadowFlagsFor(const ResolvedSceneLighting& resolved) noexcept {
    std::int32_t flags = 0;
    if (resolved.defaultShadowCast) {
        flags |= kSceneShadowCast;
    }
    if (resolved.defaultShadowReceive) {
        flags |= kSceneShadowReceive;
    }
    return flags;
}

void ApplyTimeOfDayLighting(
        const float normalizedTime,
        const SceneLightingProfile profile,
        ResolvedSceneLighting& out,
        Vector3* outSunDirectionWorld,
        Vector3* outSunColor,
        float* outSunIntensity) noexcept {
    const float t = normalizedTime - std::floor(normalizedTime);
    const float elev = SunElevation01(t);

    const bool nightInterior = (profile == SceneLightingProfile::NightInterior);
    const bool outdoor = (profile == SceneLightingProfile::Outdoor);

    float azimuth = 0.55F;
    if (outdoor) {
        azimuth = 0.35F + t * 1.1F;
    } else if (nightInterior) {
        azimuth = 0.7F;
    }

    const float cosAz = std::cos(azimuth * 6.28318530718F);
    const float sinAz = std::sin(azimuth * 6.28318530718F);
    const float cosEl = std::cos(elev * 1.37079633F);
    const float sinEl = std::sin(elev * 1.37079633F);
    Vector3 sunDir{sinAz * cosEl, sinEl, cosAz * cosEl};
    if (sunDir.LengthSquared() > 1e-8F) {
        sunDir = sunDir.Normalized();
    } else {
        sunDir = Vector3{0.35F, 0.92F, 0.18F};
    }

    const Vector3 noonColor{1.0F, 0.98F, 0.94F};
    const Vector3 warmColor{1.0F, 0.72F, 0.42F};
    const Vector3 nightColor{0.45F, 0.52F, 0.75F};
    Vector3 sunColor = Lerp3(nightColor, noonColor, elev);
    if (elev > 0.05F && elev < 0.55F) {
        const float twilight = 1.0F - std::abs(elev - 0.28F) / 0.28F;
        sunColor = Lerp3(sunColor, warmColor, Saturate(twilight) * 0.65F);
    }

    float sunIntensity = nightInterior ? 0.08F : 0.12F;
    sunIntensity += elev * (nightInterior ? 0.35F : 1.05F);

    const Vector3 daySky{0.18F, 0.22F, 0.30F};
    const Vector3 nightSky{0.04F, 0.05F, 0.09F};
    const Vector3 dayGround{0.08F, 0.075F, 0.06F};
    const Vector3 nightGround{0.025F, 0.028F, 0.04F};
    const Vector3 torchProbe{0.12F, 0.09F, 0.07F};
    const Vector3 moonProbe{0.07F, 0.08F, 0.14F};

    out.ambient.skyColor = Lerp3(nightSky, daySky, elev);
    out.ambient.groundColor = Lerp3(nightGround, dayGround, elev);
    if (nightInterior) {
        out.ambient.skyColor = nightSky;
        out.ambient.groundColor = nightGround;
        out.ambient.probeColor = Lerp3(moonProbe, torchProbe, Saturate(std::sin(t * 6.28318530718F) * 0.5F + 0.5F));
        out.ambient.probeWeight = 0.55F + 0.25F * (1.0F - elev);
        out.exposure = 0.58F + elev * 0.12F;
    } else if (outdoor) {
        out.ambient.probeColor = Lerp3(moonProbe, {0.06F, 0.065F, 0.08F}, elev);
        out.ambient.probeWeight = 0.15F + 0.12F * (1.0F - elev);
        out.exposure = 0.62F + elev * 0.18F;
    } else {
        out.ambient.probeWeight = 0.25F + 0.35F * (1.0F - elev);
    }

    if (outSunDirectionWorld != nullptr) {
        *outSunDirectionWorld = sunDir;
    }
    if (outSunColor != nullptr) {
        *outSunColor = sunColor;
    }
    if (outSunIntensity != nullptr) {
        *outSunIntensity = sunIntensity;
    }
    out.timeOfDayApplied = true;
}

ResolvedSceneLighting ResolveSceneLightingFromParams(
        const SceneLightingProfile profile,
        const float exposureOverride,
        const float shadowCascadeNearOverride,
        const float shadowCascadeFarOverride,
        const float shadowDistanceMaxOverride,
        const float shadowFadeStartRatioOverride,
        const float ambientScaleOverride,
        const bool directionalShadowsEnabled,
        const bool shadowsCastByDefault,
        const bool shadowsReceiveByDefault,
        const bool useTimeOfDay,
        const float timeOfDayNormalized) noexcept {
    const SceneLightingProfileSettings preset = LightingProfileSettingsFor(profile);
    ResolvedSceneLighting out{};
    out.exposure = PickOverride(exposureOverride, preset.exposure);
    out.shadowCascadeNear =
            (shadowCascadeNearOverride > 0.0F) ? shadowCascadeNearOverride : preset.shadowCascadeNear;
    out.shadowCascadeFar = PickOverride(shadowCascadeFarOverride, preset.shadowCascadeFar);
    out.shadowDistanceFadeEnd = PickOverride(shadowDistanceMaxOverride, preset.shadow.distanceFadeEnd);
    out.shadowDistanceFadeStartRatio =
            PickFadeStartRatio(shadowFadeStartRatioOverride, preset.shadow.distanceFadeStartRatio);
    out.shadowCascadeBlendFraction = preset.shadow.cascadeBlendFraction;
    out.directionalShadowsEnabled = directionalShadowsEnabled && preset.shadow.directionalEnabled;
    out.punctualShadowsEnabled = preset.shadow.punctualShadowsEnabled;
    out.defaultShadowCast = shadowsCastByDefault && preset.shadow.defaultCast;
    out.defaultShadowReceive = shadowsReceiveByDefault && preset.shadow.defaultReceive;
    out.ambientScale = PickOverride(ambientScaleOverride, preset.ambientScale);
    out.ambient = preset.ambient;
    out.timeOfDayApplied = useTimeOfDay || preset.timeOfDay.enabled;
    return out;
}

}  // namespace Spark
