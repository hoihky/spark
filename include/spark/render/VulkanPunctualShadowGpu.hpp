#pragma once

#include "spark/render/VulkanClusteredLightGpu.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

constexpr std::uint32_t kMaxSpotShadowMaps = 4;
constexpr std::uint32_t kMaxPointShadowMaps = 2;
constexpr std::uint32_t kSpotShadowTileSize = 512;
constexpr std::uint32_t kSpotShadowAtlasSize = 1024;
constexpr std::uint32_t kPointShadowFaceSize = 512;
constexpr std::uint32_t kPointShadowFaceCount = 6;
constexpr std::uint32_t kPointShadowLayerCount = kMaxPointShadowMaps * kPointShadowFaceCount;

/** std430 punctual shadow metadata (binding 6). Matrices/atlas UVs for spot tiles + point cubemap slices. */
struct PunctualShadowGpu {
    std::uint32_t numSpotShadows = 0;
    std::uint32_t numPointShadows = 0;
    std::uint32_t enabled = 0;
    std::uint32_t pad0 = 0;
    std::int32_t spotLightIndex[kMaxSpotShadowMaps]{};
    std::int32_t pointLightIndex[kMaxPointShadowMaps]{};
    std::int32_t pad1[2]{};
    float spotWorldToClip[kMaxSpotShadowMaps][16]{};
    float spotAtlas[kMaxSpotShadowMaps][4]{};
    float pointPosRange[kMaxPointShadowMaps][4]{};
    std::uint32_t pointBaseLayer[kMaxPointShadowMaps]{};
    std::uint32_t pad2[2]{};
    float pointFaceWorldToClip[kMaxPointShadowMaps][kPointShadowFaceCount][16]{};
    /** Per-scene-light index → shadow map slot, or -1 (matches clustered light SSBO indices). */
    std::int32_t pointShadowSlotByLight[kMaxClusteredPointLights]{};
    std::int32_t spotShadowSlotByLight[kMaxClusteredSpotLights]{};
};

inline constexpr std::uint32_t kPunctualShadowGpuBytes = static_cast<std::uint32_t>(sizeof(PunctualShadowGpu));

static_assert(
        offsetof(PunctualShadowGpu, pointFaceWorldToClip) == 416U,
        "PunctualShadowGpu layout must match punctual_shadows.glsl std430");
static_assert(
        offsetof(PunctualShadowGpu, pointShadowSlotByLight) == 1184U,
        "PunctualShadowGpu layout must match punctual_shadows.glsl std430");

}  // namespace Spark
