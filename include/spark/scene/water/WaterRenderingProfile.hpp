#pragma once

#include "spark/scene/water/WaterScreenSpaceReflectionSettings.hpp"

#include <cstdint>

namespace Spark {

/**
 * Global water rendering quality preset (mirrors <c>SceneLightingProfile</c>).
 * Applied per frame via <c>SceneRenderParams::waterRenderingProfile</c>.
 */
enum class WaterRenderingProfile : std::uint8_t {
    /** Tier B: full-res SSR, up to 24 ray steps. */
    Default = 0,
    /** Half-res SSR sampling and capped ray steps for mid-tier hardware. */
    Balanced = 1,
    /** Tier A / mobile min-spec: SSR off; sky IBL reflection only. */
    MinSpec = 2,
};

/** Per-profile SSR performance budget (W3-06). */
struct WaterRenderingProfileSettings {
    bool ssrEnabled = true;
    bool ssrHalfRes = false;
    std::int32_t ssrMaxStepsCap = WaterScreenSpaceReflectionSettings::kDefaultMaxSteps;
};

/** GPU-ready SSR values after merging body settings with the active profile. */
struct ResolvedWaterScreenSpaceReflection {
    bool enabled = true;
    bool halfRes = false;
    std::int32_t maxSteps = WaterScreenSpaceReflectionSettings::kDefaultMaxSteps;
    float maxRayDistance = WaterScreenSpaceReflectionSettings::kDefaultMaxRayDistance;
    float thickness = WaterScreenSpaceReflectionSettings::kDefaultThickness;
    float stepScale = WaterScreenSpaceReflectionSettings::kDefaultStepScale;
    float strength = WaterScreenSpaceReflectionSettings::kDefaultStrength;
    float horizonFadeStart = WaterScreenSpaceReflectionSettings::kDefaultHorizonFadeStart;
    float horizonFadeEnd = WaterScreenSpaceReflectionSettings::kDefaultHorizonFadeEnd;
};

[[nodiscard]] WaterRenderingProfileSettings WaterRenderingProfileSettingsFor(WaterRenderingProfile profile) noexcept;

/** Merges per-body SSR tuning with the frame profile budget. */
[[nodiscard]] ResolvedWaterScreenSpaceReflection ResolveWaterScreenSpaceReflection(
        const WaterScreenSpaceReflectionSettings& body,
        WaterRenderingProfile profile) noexcept;

}  // namespace Spark
