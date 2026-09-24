#include "spark/render/lighting/SceneLightingSetup.hpp"

#include "spark/engine/SceneRenderParams.hpp"

namespace Spark {

SceneLightingSetup SceneLightingSetup::FromProfile(const SceneLightingProfile profile) noexcept {
    return SceneLightingSetup{profile};
}

SceneLightingSetup::SceneLightingSetup(const SceneLightingProfile profile) noexcept : profile_(profile) {}

SceneLightingSetup& SceneLightingSetup::WithExposure(const float exposureMultiplier) noexcept {
    exposure_ = exposureMultiplier;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithShadowCascades(const float nearPlane, const float farPlane) noexcept {
    cascadeNear_ = nearPlane;
    cascadeFar_ = farPlane;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithShadowDistanceFade(
        const float endWorldMeters,
        const float startRatio) noexcept {
    shadowDistanceMax_ = endWorldMeters;
    shadowFadeStartRatio_ = startRatio;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithDirectionalShadows(const bool enabled) noexcept {
    directionalShadows_ = enabled;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithPunctualShadows(const bool enabled) noexcept {
    punctualShadows_ = enabled;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithDefaultShadowParticipation(
        const bool castByDefault,
        const bool receiveByDefault) noexcept {
    castByDefault_ = castByDefault;
    receiveByDefault_ = receiveByDefault;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithTimeOfDay(const bool enabled, const float normalizedTime) noexcept {
    useTimeOfDay_ = enabled;
    timeOfDay_ = normalizedTime;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithSsao(const bool enabled) noexcept {
    ssaoEnabled_ = enabled;
    return *this;
}

SceneLightingSetup& SceneLightingSetup::WithAmbientScale(const float scale) noexcept {
    ambientScale_ = scale;
    return *this;
}

void SceneLightingSetup::ApplyTo(SceneRenderParams& params) const noexcept {
    params.lightingProfile = profile_;
    if (exposure_.HasValue()) {
        params.exposure = *exposure_;
    }
    if (cascadeNear_.HasValue()) {
        params.shadowCascadeNear = *cascadeNear_;
    }
    if (cascadeFar_.HasValue()) {
        params.shadowCascadeFar = *cascadeFar_;
    }
    if (shadowDistanceMax_.HasValue()) {
        params.shadowDistanceMax = *shadowDistanceMax_;
    }
    if (shadowFadeStartRatio_.HasValue()) {
        params.shadowFadeStartRatio = *shadowFadeStartRatio_;
    }
    if (directionalShadows_.HasValue()) {
        params.directionalShadowsEnabled = *directionalShadows_;
    }
    if (punctualShadows_.HasValue()) {
        params.punctualShadowsEnabled = *punctualShadows_;
    }
    if (castByDefault_.HasValue()) {
        params.shadowsCastByDefault = *castByDefault_;
    }
    if (receiveByDefault_.HasValue()) {
        params.shadowsReceiveByDefault = *receiveByDefault_;
    }
    if (useTimeOfDay_.HasValue()) {
        params.useTimeOfDay = *useTimeOfDay_;
    }
    if (timeOfDay_.HasValue()) {
        params.timeOfDay = *timeOfDay_;
    }
    if (ssaoEnabled_.HasValue()) {
        params.ssaoEnabled = *ssaoEnabled_;
    }
    if (ambientScale_.HasValue()) {
        params.ambientScale = *ambientScale_;
    }
}

}  // namespace Spark
