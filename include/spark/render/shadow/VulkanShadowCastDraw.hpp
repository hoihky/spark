#pragma once

#include "spark/render/shadow/VulkanDirectionalShadowPass.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

struct VulkanShadowPushConstants {
    float model[16]{};
    float lightViewProj[16]{};
    std::int32_t useSkinning = 0;
    std::int32_t jointCount = 0;
    std::int32_t sp0 = 0;
    std::int32_t sp1 = 0;
};

/** Draw all shadow-casting scene meshes for one light tile/face (shared by directional + punctual passes). */
void RecordShadowCastMeshes(
        VkCommandBuffer commandBuffer,
        VkPipeline pipeline,
        VkPipelineLayout pipelineLayout,
        const VulkanShadowRecordContext& ctx,
        const float lightViewProj[16],
        std::uint32_t frameIndex);

}  // namespace Spark
