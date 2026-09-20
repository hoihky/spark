#include "spark/scene/water/WaterRenderingProfile.hpp"

#include <algorithm>

namespace Spark {

WaterRenderingProfileSettings WaterRenderingProfileSettingsFor(const WaterRenderingProfile profile) noexcept {
    WaterRenderingProfileSettings settings{};
    switch (profile) {
    case WaterRenderingProfile::Balanced:
        settings.ssrEnabled = true;
        settings.ssrHalfRes = true;
        settings.ssrMaxStepsCap = 12;
        break;
    case WaterRenderingProfile::MinSpec:
        settings.ssrEnabled = false;
        settings.ssrHalfRes = false;
        settings.ssrMaxStepsCap = 0;
        break;
    case WaterRenderingProfile::Default:
    default:
        settings.ssrEnabled = true;
        settings.ssrHalfRes = false;
        settings.ssrMaxStepsCap = WaterScreenSpaceReflectionSettings::kDefaultMaxSteps;
        break;
    }
    return settings;
}

ResolvedWaterScreenSpaceReflection ResolveWaterScreenSpaceReflection(
        const WaterScreenSpaceReflectionSettings& body,
        const WaterRenderingProfile profile) noexcept {
    const WaterRenderingProfileSettings preset = WaterRenderingProfileSettingsFor(profile);
    ResolvedWaterScreenSpaceReflection resolved{};
    resolved.enabled = body.IsEnabled() && preset.ssrEnabled;
    resolved.halfRes = preset.ssrHalfRes;
    resolved.maxRayDistance = body.GetMaxRayDistance();
    resolved.thickness = body.GetThickness();
    resolved.stepScale = body.GetStepScale();
    resolved.strength = body.GetStrength();
    resolved.horizonFadeStart = body.GetHorizonFadeStart();
    resolved.horizonFadeEnd = body.GetHorizonFadeEnd();

    if (!preset.ssrEnabled) {
        resolved.maxSteps = 0;
        return resolved;
    }

    const std::int32_t bodySteps = body.GetMaxSteps();
    const std::int32_t capped =
            preset.ssrMaxStepsCap > 0 ? std::min(bodySteps, preset.ssrMaxStepsCap) : bodySteps;
    resolved.maxSteps = capped > 0 ? capped : 1;
    return resolved;
}

}  // namespace Spark
