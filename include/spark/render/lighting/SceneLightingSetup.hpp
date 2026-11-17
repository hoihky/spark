#pragma once

#include "spark/core/Optional.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"

namespace Spark {

struct SceneRenderParams;

/**
 * Fluent, immutable-style configuration for <c>SceneRenderParams</c> lighting fields.
 *
 * Apply before <c>FillStandardLitSceneFromWorld</c> so <c>SceneLightingResolver</c> sees profile overrides.
 * Does not touch draw lists or camera matrices.
 */
class SceneLightingSetup {
public:
    [[nodiscard]] static SceneLightingSetup FromProfile(const SceneLightingProfile profile) noexcept;

    SceneLightingSetup& WithExposure(const float exposureMultiplier) noexcept;
    SceneLightingSetup& WithShadowCascades(const float nearPlane, const float farPlane) noexcept;
    SceneLightingSetup& WithShadowDistanceFade(const float endWorldMeters, const float startRatio = 0.82F) noexcept;
    SceneLightingSetup& WithDirectionalShadows(const bool enabled) noexcept;
    SceneLightingSetup& WithPunctualShadows(const bool enabled) noexcept;
    SceneLightingSetup& WithDefaultShadowParticipation(const bool castByDefault, const bool receiveByDefault) noexcept;
    SceneLightingSetup& WithTimeOfDay(const bool enabled, const float normalizedTime = 0.5F) noexcept;
    SceneLightingSetup& WithSsao(const bool enabled) noexcept;
    SceneLightingSetup& WithAmbientScale(const float scale) noexcept;

    void ApplyTo(SceneRenderParams& params) const noexcept;

    [[nodiscard]] SceneLightingProfile GetProfile() const noexcept { return profile; }

private:
    explicit SceneLightingSetup(const SceneLightingProfile profile) noexcept;

    SceneLightingProfile profile = SceneLightingProfile::Default;
    Optional<float> exposure;
    Optional<float> cascadeNear;
    Optional<float> cascadeFar;
    Optional<float> shadowDistanceMax;
    Optional<float> shadowFadeStartRatio;
    Optional<bool> directionalShadows;
    Optional<bool> punctualShadows;
    Optional<bool> castByDefault;
    Optional<bool> receiveByDefault;
    Optional<bool> useTimeOfDay;
    Optional<float> timeOfDay;
    Optional<bool> ssaoEnabled;
    Optional<float> ambientScale;
};

}  // namespace Spark
