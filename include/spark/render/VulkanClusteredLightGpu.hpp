#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <cstdint>

namespace Spark {

/** 3D screen-depth cluster grid for forward punctual lighting (matches clustered_lights.glsl). */
constexpr std::uint32_t kClusterGridX = 16;
constexpr std::uint32_t kClusterGridY = 16;
constexpr std::uint32_t kClusterGridZ = 16;
constexpr std::uint32_t kClusterCount = kClusterGridX * kClusterGridY * kClusterGridZ;
constexpr std::uint32_t kMaxClusteredPointLights = 256;
constexpr std::uint32_t kMaxClusteredSpotLights = 128;
constexpr std::uint32_t kMaxClusterLightIndices = 16384;
constexpr std::uint32_t kMaxLightsPerCluster = 128;

static_assert(SceneRenderParams::MaxPointLights == kMaxClusteredPointLights);
static_assert(SceneRenderParams::MaxSpotLights == kMaxClusteredSpotLights);

/** std430 lights SSBO (binding 4). */
struct ClusterLightsHeaderGpu {
    std::uint32_t numPointLights = 0;
    std::uint32_t numSpotLights = 0;
    std::uint32_t pad0 = 0;
    std::uint32_t pad1 = 0;
};

struct ClusterLightsGpu {
    ClusterLightsHeaderGpu header{};
    float pointPositionRange[kMaxClusteredPointLights][4]{};
    float pointColorIntensity[kMaxClusteredPointLights][4]{};
    float spotPositionRange[kMaxClusteredSpotLights][4]{};
    float spotDirectionCosOuter[kMaxClusteredSpotLights][4]{};
    float spotColorIntensity[kMaxClusteredSpotLights][4]{};
    float spotCosInner[kMaxClusteredSpotLights][4]{};
};

inline constexpr std::uint32_t kClusterLightsGpuBytes = static_cast<std::uint32_t>(sizeof(ClusterLightsGpu));

/** std430 cluster index SSBO (binding 5). */
struct ClusterGridGpu {
    std::uint32_t offsets[kClusterCount]{};
    std::uint32_t counts[kClusterCount]{};
    std::uint32_t indices[kMaxClusterLightIndices]{};
};

inline constexpr std::uint32_t kClusterGridGpuBytes = static_cast<std::uint32_t>(sizeof(ClusterGridGpu));

}  // namespace Spark
