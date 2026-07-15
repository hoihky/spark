#include "spark/render/VulkanPresentationFramebuffers.hpp"

#include <stdexcept>

namespace Spark {

void VulkanPresentationFramebuffers::Create(
        VkDevice device,
        VkRenderPass renderPass,
        VkExtent2D extent,
        const Array<VkImageView>& colorImageViews,
        VkImageView depthImageView) {
    Destroy(device);
    const bool withDepth = (depthImageView != VK_NULL_HANDLE);
    buffers.Resize(colorImageViews.GetSize());
    for (std::size_t i = 0; i < colorImageViews.GetSize(); ++i) {
        VkFramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;
        if (withDepth) {
            const VkImageView attachments[] = {colorImageViews[i], depthImageView};
            framebufferCreateInfo.attachmentCount = 2;
            framebufferCreateInfo.pAttachments = attachments;
        } else {
            framebufferCreateInfo.attachmentCount = 1;
            framebufferCreateInfo.pAttachments = &colorImageViews[i];
        }
        framebufferCreateInfo.width = extent.width;
        framebufferCreateInfo.height = extent.height;
        framebufferCreateInfo.layers = 1;
        if (vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateFramebuffer failed");
        }
    }
}

void VulkanPresentationFramebuffers::Destroy(VkDevice device) noexcept {
    for (std::size_t fi = 0; fi < buffers.GetSize(); ++fi) {
        vkDestroyFramebuffer(device, buffers[fi], nullptr);
    }
    buffers.Clear();
}

}  // namespace Spark
