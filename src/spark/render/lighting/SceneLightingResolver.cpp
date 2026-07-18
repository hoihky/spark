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
            params.shadowsCastByDefault,
            params.shadowsReceiveByDefault,
            params.useTimeOfDay,
            params.timeOfDay);

    if (resolved.timeOfDayApplied || params.useTimeOfDay) {
        const SceneLightingProfileSettings preset = LightingProfileSettingsFor(params.lightingProfile);
        const float tod = params.useTimeOfDay ? params.timeOfDay : preset.timeOfDay.normalizedTime;
        ApplyTimeOfDayLighting(
                tod,
                params.lightingProfile,
                resolved,
                &params.lightDirectionWorld,
                &params.lightColor,
                &params.lightIntensity);
    }

    if (params.ambientColor.x > 0.001F || params.ambientColor.y > 0.001F || params.ambientColor.z > 0.001F) {
        resolved.ambient.groundColor = params.ambientColor;
    }

    return resolved;
}

}  // namespace Spark
