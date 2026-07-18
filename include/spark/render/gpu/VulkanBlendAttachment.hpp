#pragma once

#include "spark/render/scene/SceneBlendMode.hpp"

#include <vulkan/vulkan.h>

namespace Spark {

/** Full RGBA write mask used by UI and sprite blend attachments. */
inline constexpr VkColorComponentFlags kVulkanBlendColorWriteMaskRgba =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

/** Full RGB write mask used by the HDR sprite pass (alpha stored implicitly). */
inline constexpr VkColorComponentFlags kVulkanBlendColorWriteMaskRgb =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;

/** Fills <c>out</c> with Vulkan fixed-function blend factors for <c>mode</c>. */
void VulkanBlendAttachmentForMode(SceneBlendMode mode, VkPipelineColorBlendAttachmentState& out) noexcept;

/**
 * Creates one graphics pipeline from <c>baseInfo</c> with the blend attachment for <c>mode</c>.
 * <c>baseInfo.pColorBlendState</c> must point at a struct whose <c>pAttachments</c> has room for one entry.
 */
[[nodiscard]] VkResult VulkanCreateGraphicsPipelineForBlendMode(
        VkDevice device,
        const VkGraphicsPipelineCreateInfo& baseInfo,
        SceneBlendMode mode,
        VkPipeline* outPipeline) noexcept;

}  // namespace Spark
