#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <cstdint>

namespace Spark {

/** std140 UBO: directional + shadow + cluster grid metadata (punctual lights live in SSBOs). */
struct SceneUniformGpu {
    float viewProj[16]{};
    float lightDir[4]{};
    float cameraPos[4]{};
    float lightColor[4]{};
    float ambientColor[4]{};
    float ambientSky[4]{};
    float ambientProbe[4]{};
    float invViewProj[16]{};
    float viewportSize[4]{};
    float timeGlobal[4]{};
    float worldToShadowClip[4][16]{};
    float cascadeSplits[4]{};
    float cascadeAtlas[4][4]{};
    float shadowParams[4]{};
    /** xyz = cluster grid dimensions; w = num point lights (for shader index decode). */
    float clusterGrid[4]{};
    /** x/y = logarithmic depth slice near/far; z = num spot lights; w = clustered enabled (1). */
    float clusterDepth[4]{};
    /** x = env equirect layer (-1 = procedural); y = IBL intensity; w = enabled (1). */
    float iblParams[4]{};
};

static_assert(sizeof(SceneUniformGpu) == 656);

inline constexpr std::uint32_t kSceneUniformGpuBytes = 656;

}  // namespace Spark
