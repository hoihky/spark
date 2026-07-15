#pragma once

#include "spark/math/Matrix4.hpp"

namespace Spark {

/** Per-frame CSM data produced by <c>VulkanSceneUniformWriter</c> and consumed by the shadow pass. */
struct VulkanDirectionalShadowFrameState {
    bool active = false;
    Matrix4 worldToShadowClip[4]{};
    float cascadeSplits[4]{};
    float cascadeAtlas[4][4]{};
};

}  // namespace Spark
