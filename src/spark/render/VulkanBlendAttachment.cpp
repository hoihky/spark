#include "spark/render/VulkanBlendAttachment.hpp"

namespace Spark {

void VulkanBlendAttachmentForMode(const SceneBlendMode mode, VkPipelineColorBlendAttachmentState& out) noexcept {
    out = {};
    out.colorWriteMask = kVulkanBlendColorWriteMaskRgba;
    out.blendEnable = SceneBlendModeUsesAlphaBlend(mode) ? VK_TRUE : VK_FALSE;
    out.colorBlendOp = VK_BLEND_OP_ADD;
    out.alphaBlendOp = VK_BLEND_OP_ADD;

    switch (mode) {
    case SceneBlendMode::Opaque:
        out.blendEnable = VK_FALSE;
        break;
    case SceneBlendMode::PremultipliedAlpha:
        out.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        out.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        out.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        out.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case SceneBlendMode::Multiply:
        out.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
        out.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        out.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        out.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        break;
    case SceneBlendMode::Screen:
        out.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        out.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        out.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        out.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case SceneBlendMode::Additive:
        out.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        out.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        out.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        out.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        break;
    case SceneBlendMode::AlphaOver:
    default:
        out.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        out.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        out.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        out.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        break;
    }
}

VkResult VulkanCreateGraphicsPipelineForBlendMode(
        const VkDevice device,
        const VkGraphicsPipelineCreateInfo& baseInfo,
        const SceneBlendMode mode,
        VkPipeline* const outPipeline) noexcept {
    if (outPipeline == nullptr || baseInfo.pColorBlendState == nullptr ||
        baseInfo.pColorBlendState->pAttachments == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    VkPipelineColorBlendAttachmentState blendAtt = *baseInfo.pColorBlendState->pAttachments;
    VulkanBlendAttachmentForMode(mode, blendAtt);

    VkPipelineColorBlendStateCreateInfo blendState = *baseInfo.pColorBlendState;
    blendState.pAttachments = &blendAtt;

    VkGraphicsPipelineCreateInfo pipeInfo = baseInfo;
    pipeInfo.pColorBlendState = &blendState;
    return vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, outPipeline);
}

}  // namespace Spark
