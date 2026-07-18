#pragma once

#include "spark/math/Matrix4.hpp"
#include "spark/render/shadow/VulkanPunctualShadowGpu.hpp"

#include <cstdint>

namespace Spark {

/** Per-frame spot/point shadow selection and matrices (CPU mirror of <c>PunctualShadowGpu</c>). */
struct VulkanPunctualShadowFrameState {
    bool active = false;
    std::uint32_t numSpotShadows = 0;
    std::uint32_t numPointShadows = 0;
    std::int32_t spotLightIndex[kMaxSpotShadowMaps]{};
    std::int32_t pointLightIndex[kMaxPointShadowMaps]{};
    Matrix4 spotWorldToClip[kMaxSpotShadowMaps]{};
    float spotAtlas[kMaxSpotShadowMaps][4]{};
    float pointPosRange[kMaxPointShadowMaps][4]{};
    std::uint32_t pointBaseLayer[kMaxPointShadowMaps]{};
    Matrix4 pointFaceViewProj[kMaxPointShadowMaps][kPointShadowFaceCount]{};
};

}  // namespace Spark
