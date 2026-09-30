#pragma once

#include "spark/engine/SceneRenderParams.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/** std430 layout for <c>shaders/foliage_instance.glsl</c>. */
class VulkanFoliageInstanceGpu {
public:
    float model[16]{};
    float tint[4]{1.0F, 1.0F, 1.0F, 1.0F};
    float windPhase = 0.0F;
    float padding[3]{};
};

static_assert(sizeof(VulkanFoliageInstanceGpu) == 96U);

constexpr std::uint32_t kMaxFoliageInstancesGpu = SceneRenderParams::MaxFoliageInstances;
constexpr std::size_t kFoliageInstanceSsboBytes =
        sizeof(VulkanFoliageInstanceGpu) * static_cast<std::size_t>(kMaxFoliageInstancesGpu);

}  // namespace Spark
