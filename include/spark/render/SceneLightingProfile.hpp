#pragma once

#include "spark/math/Vector3.hpp"

#include <cstdint>

namespace Spark {

/** Per-draw shadow participation (packed in <c>SceneDrawItem::shadowFlags</c> / push constants). */
inline constexpr std::int32_t kSceneShadowCast = 1;
inline constexpr std::int32_t kSceneShadowReceive = 2;
inline constexpr std::int32_t kSceneShadowCastAndReceive = kSceneShadowCast | kSceneShadowReceive;

/**
 * Preset lighting/shadow tuning for outdoor vistas vs enclosed combat vs night interiors.
 * Resolved with optional overrides on <c>SceneRenderParams</c> via <c>ResolveSceneLightingFromParams</c>.
 */
enum class SceneLightingProfile : std::uint8_t {
    /** Balanced defaults (long cascades, no distance fade unless overridden). */
    Default = 0,
    /** Open world: long shadow fade, sun arc via time-of-day, cool sky / warm ground hemisphere. */
    Outdoor = 1,
    /** Combat spaces: tighter cascades, shorter fade, brighter fill. */
    Interior = 2,
    /** Dim moonlit fill + probe; weak sun; for dungeons at night. */
    NightInterior = 3,
};

/** Shared shadow tuning (profile + optional <c>SceneRenderParams</c> overrides). */
struct SceneShadowSettings {
    bool directionalEnabled = true;
    bool punctualShadowsEnabled = true;
    /** World-space distance where directional shadows fully fade out; 0 = no fade. */
    float distanceFadeEnd = 0.0F;
    /** Fade begins at <c>distanceFadeEnd * distanceFadeStartRatio</c>. */
    float distanceFadeStartRatio = 0.82F;
    /**
     * Fraction of each cascade's far split distance used as a soft blend band into the next cascade (0–1).
     * Typical 0.08–0.15 hides CSM tile boundaries.
     */
    float cascadeBlendFraction = 0.12F;
    /** Default for draws that do not set <c>SceneDrawItem::shadowFlags</c> explicitly. */
    bool defaultCast = true;
    bool defaultReceive = true;
};

/** Hemisphere + uniform probe fill (packed into scene UBO). */
struct SceneAmbientSettings {
    Vector3 groundColor{0.05F, 0.055F, 0.07F};
    Vector3 skyColor{0.12F, 0.14F, 0.18F};
    /** Uniform ambient term (night interiors, base fill). .w on GPU = weight. */
    Vector3 probeColor{0.04F, 0.045F, 0.06F};
    float probeWeight = 0.35F;
};

/**
 * Normalized day phase: 0 = midnight, 0.25 = sunrise, 0.5 = solar noon, 0.75 = sunset.
 * Outdoor / night-interior presets use this to drive sun and ambient when enabled on params.
 */
struct SceneTimeOfDaySettings {
    bool enabled = false;
    float normalizedTime = 0.5F;
};

/** Full preset table row (internal defaults per <c>SceneLightingProfile</c>). */
struct SceneLightingProfileSettings {
    float exposure = 0.68F;
    float shadowCascadeNear = 0.12F;
    float shadowCascadeFar = 400.0F;
    float ambientScale = 0.88F;
    SceneShadowSettings shadow{};
    SceneAmbientSettings ambient{};
    SceneTimeOfDaySettings timeOfDay{};
};

/** Resolved per-frame values after profile + param overrides + optional time-of-day. */
struct ResolvedSceneLighting {
    float exposure = 0.68F;
    float shadowCascadeNear = 0.12F;
    float shadowCascadeFar = 400.0F;
    float shadowDistanceFadeEnd = 0.0F;
    float shadowDistanceFadeStartRatio = 0.82F;
    float shadowCascadeBlendFraction = 0.12F;
    bool directionalShadowsEnabled = true;
    bool punctualShadowsEnabled = true;
    bool defaultShadowCast = true;
    bool defaultShadowReceive = true;
    float ambientScale = 0.88F;
    SceneAmbientSettings ambient{};
    bool timeOfDayApplied = false;
};

[[nodiscard]] SceneLightingProfileSettings LightingProfileSettingsFor(SceneLightingProfile profile) noexcept;

[[nodiscard]] std::int32_t DefaultShadowFlagsFor(const ResolvedSceneLighting& resolved) noexcept;

/**
 * Applies analytic sun + hemisphere + probe for <c>normalizedTime</c> on top of resolved preset ambient.
 * Mutates <c>out</c> and optionally fills sun direction/color/intensity for the scene pass.
 */
void ApplyTimeOfDayLighting(
        float normalizedTime,
        SceneLightingProfile profile,
        ResolvedSceneLighting& out,
        Vector3* outSunDirectionWorld,
        Vector3* outSunColor,
        float* outSunIntensity) noexcept;

/** Combines profile defaults with non-zero overrides on <c>SceneRenderParams</c>. */
[[nodiscard]] ResolvedSceneLighting ResolveSceneLightingFromParams(
        SceneLightingProfile profile,
        float exposureOverride,
        float shadowCascadeNearOverride,
        float shadowCascadeFarOverride,
        float shadowDistanceMaxOverride,
        float shadowFadeStartRatioOverride,
        float ambientScaleOverride,
        bool directionalShadowsEnabled,
        bool shadowsCastByDefault,
        bool shadowsReceiveByDefault,
        bool useTimeOfDay,
        float timeOfDayNormalized) noexcept;

}  // namespace Spark
