#include "spark/render/lighting/SceneLightingSetup.hpp"

#include "spark/engine/SceneRenderParams.hpp"

namespace Spark {

SceneLightingSetup SceneLightingSetup::FromProfile(const SceneLightingProfile profile) noexcept {
    return SceneLightingSetup{profile};
}

SceneLightingSetup::SceneLightingSetup(const SceneLightingProfile initialProfile) noexcept
    : profile(initialProfile) {}

SceneLightingSetup& SceneLightingSetup::WithExposure(const float exposureMultiplier) noexcept {
    exposure = exposureMultiplier;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithShadowCascades(const float nearPlane, const float farPlane) noexcept {
    cascadeNear = nearPlane;
    cascadeFar = farPlane;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithShadowDistanceFade(
        const float endWorldMeters,
        const float startRatio) noexcept {
    shadowDistanceMax = endWorldMeters;
    shadowFadeStartRatio = startRatio;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithDirectionalShadows(const bool enabled) noexcept {
    directionalShadows = enabled;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithPunctualShadows(const bool enabled) noexcept {
    punctualShadows = enabled;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithDefaultShadowParticipation(
        const bool castDefault,
        const bool receiveDefault) noexcept {
    castByDefault = castDefault;
    receiveByDefault = receiveDefault;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithTimeOfDay(const bool enabled, const float normalizedTime) noexcept {
    useTimeOfDay = enabled;
    timeOfDay = normalizedTime;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithSsao(const bool enabled) noexcept {
    ssaoEnabled = enabled;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithAmbientScale(const float scale) noexcept {
    ambientScale = scale;
    return *this;
}

void SceneLightingSetup::ApplyTo(SceneRenderParams& params) const noexcept {
    params.lightingProfile = profile;
    if (exposure.HasValue()) {
        params.exposure = *exposure;
    }
    if (cascadeNear.HasValue()) {
        params.shadowCascadeNear = *cascadeNear;
    }
    if (cascadeFar.HasValue()) {
        params.shadowCascadeFar = *cascadeFar;
    }
    if (shadowDistanceMax.HasValue()) {
        params.shadowDistanceMax = *shadowDistanceMax;
    }
    if (shadowFadeStartRatio.HasValue()) {
        params.shadowFadeStartRatio = *shadowFadeStartRatio;
    }
    if (directionalShadows.HasValue()) {
        params.directionalShadowsEnabled = *directionalShadows;
    }
    if (punctualShadows.HasValue()) {
        params.punctualShadowsEnabled = *punctualShadows;
    }
    if (castByDefault.HasValue()) {
        params.shadowsCastByDefault = *castByDefault;
    }
    if (receiveByDefault.HasValue()) {
        params.shadowsReceiveByDefault = *receiveByDefault;
    }
    if (useTimeOfDay.HasValue()) {
        params.useTimeOfDay = *useTimeOfDay;
    }
    if (timeOfDay.HasValue()) {
        params.timeOfDay = *timeOfDay;
    }
    if (ssaoEnabled.HasValue()) {
        params.ssaoEnabled = *ssaoEnabled;
    }
    if (ambientScale.HasValue()) {
        params.ambientScale = *ambientScale;
    }
}

}  // namespace Spark
