#include "spark/render/VulkanPresentationSwapchain.hpp"

#include <cstddef>

namespace Spark {

void VulkanPresentationSwapchain::Destroy(VkDevice device) noexcept {
    for (std::size_t vi = 0; vi < imageViews.GetSize(); ++vi) {
        vkDestroyImageView(device, imageViews[vi], nullptr);
    }
    imageViews.Clear();
    if (khr != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, khr, nullptr);
        khr = VK_NULL_HANDLE;
    }
    images.Clear();
}

}  // namespace Spark
