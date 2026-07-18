#pragma once

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"

namespace Spark {

/**
 * Resolves <c>SceneRenderParams</c> lighting profile overrides, time-of-day, and ambient tint
 * into <c>ResolvedSceneLighting</c> for submit and GPU upload paths.
 */
class SceneLightingResolver {
public:
    /**
     * Combines profile defaults with param overrides and optional time-of-day.
     * May update sun direction/color/intensity on <c>params</c> when time-of-day is active.
     */
    [[nodiscard]] static ResolvedSceneLighting Resolve(SceneRenderParams& params) noexcept;
};

}  // namespace Spark
