#include "spark/render/lighting/SceneLightingResolver.hpp"

namespace Spark {

ResolvedSceneLighting SceneLightingResolver::Resolve(SceneRenderParams& params) noexcept {
    ResolvedSceneLighting resolved = ResolveSceneLightingFromParams(
            params.lightingProfile,
            params.exposure,
            params.shadowCascadeNear,
            params.shadowCascadeFar,
            params.shadowDistanceMax,
            params.shadowFadeStartRatio,
            params.ambientScale,
            params.directionalShadowsEnabled,
            params.punctualShadowsEnabled,
            params.shadowsCastByDefault,
            params.shadowsReceiveByDefault,
            params.useTimeOfDay,
            params.timeOfDay);

    const bool exposureOverridden = params.exposure > 0.0F;
    const float savedExposure = resolved.exposure;

    if (params.useTimeOfDay) {
        ApplyTimeOfDayLighting(
                params.timeOfDay,
                params.lightingProfile,
                resolved,
                &params.lightDirectionWorld,
                &params.lightColor,
                &params.lightIntensity);
        if (exposureOverridden) {
            resolved.exposure = savedExposure;
        }
    }

    if (!params.useTimeOfDay &&
        (params.ambientColor.x > 0.001F || params.ambientColor.y > 0.001F || params.ambientColor.z > 0.001F)) {
        resolved.ambient.groundColor = params.ambientColor;
    }

    return resolved;
}

}  // namespace Spark
