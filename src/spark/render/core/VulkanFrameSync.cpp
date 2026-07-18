#include "spark/render/core/VulkanFrameSync.hpp"

#include <stdexcept>

namespace Spark {

void VulkanFrameSync::Create(VkDevice device) {
    imageAvailableSemaphores.Resize(kMaxFramesInFlight);
    inFlightFences.Resize(kMaxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (std::size_t i = 0; i < kMaxFramesInFlight; ++i) {
        if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("VulkanFrameSync::Create failed");
        }
    }
}

void VulkanFrameSync::DestroyFlightSync(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (std::size_t i = 0; i < imageAvailableSemaphores.GetSize(); ++i) {
        if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            imageAvailableSemaphores[i] = VK_NULL_HANDLE;
        }
    }
    imageAvailableSemaphores.Clear();

    for (std::size_t i = 0; i < inFlightFences.GetSize(); ++i) {
        if (inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, inFlightFences[i], nullptr);
            inFlightFences[i] = VK_NULL_HANDLE;
        }
    }
    inFlightFences.Clear();
    imagesInFlight.Clear();
    currentFrame = 0;
}

void VulkanFrameSync::DestroySwapchainSync(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (std::size_t i = 0; i < renderFinishedSemaphores.GetSize(); ++i) {
        if (renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }
    }
    renderFinishedSemaphores.Clear();
}

void VulkanFrameSync::RecreateSwapchainSync(VkDevice device, std::size_t swapchainImageCount) {
    DestroySwapchainSync(device);

    renderFinishedSemaphores.Resize(swapchainImageCount);

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (std::size_t i = 0; i < swapchainImageCount; ++i) {
        if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("VulkanFrameSync::RecreateSwapchainSync failed");
        }
    }

    imagesInFlight.Resize(swapchainImageCount);
    for (std::size_t ii = 0; ii < imagesInFlight.GetSize(); ++ii) {
        imagesInFlight[ii] = VK_NULL_HANDLE;
    }
}

void VulkanFrameSync::WaitForCurrentFrameFence(VkDevice device) const {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
}

VkResult VulkanFrameSync::AcquireNextImage(
        VkDevice device,
        VkSwapchainKHR swapchain,
        std::uint32_t& outImageIndex) const {
    return vkAcquireNextImageKHR(
            device,
            swapchain,
            UINT64_MAX,
            imageAvailableSemaphores[currentFrame],
            VK_NULL_HANDLE,
            &outImageIndex);
}

void VulkanFrameSync::WaitForSwapchainImageFence(VkDevice device, std::uint32_t imageIndex) const {
    if (imageIndex < imagesInFlight.GetSize() && imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
}

void VulkanFrameSync::TrackSwapchainImageInFlight(std::uint32_t imageIndex) noexcept {
    if (imageIndex < imagesInFlight.GetSize()) {
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];
    }
}

void VulkanFrameSync::WaitForAllOtherFrames(VkDevice device) const {
    // Caller must have already waited inFlightFences[currentFrame] this frame and not reset it yet.
    for (std::size_t i = 0; i < inFlightFences.GetSize(); ++i) {
        if (i == static_cast<std::size_t>(currentFrame)) {
            continue;
        }
        vkWaitForFences(device, 1, &inFlightFences[i], VK_TRUE, UINT64_MAX);
    }
}

void VulkanFrameSync::ResetCurrentFrameFence(VkDevice device) const {
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
}

void VulkanFrameSync::SubmitFrame(
        VkQueue graphicsQueue,
        VkCommandBuffer commandBuffer,
        std::uint32_t imageIndex) const {
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    constexpr VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("vkQueueSubmit failed");
    }
}

void VulkanFrameSync::WaitForSubmittedFrame(VkDevice device) const {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
}

VkResult VulkanFrameSync::PresentFrame(
        VkQueue presentQueue,
        VkSwapchainKHR swapchain,
        std::uint32_t imageIndex) const {
    VkSwapchainKHR swapchains[] = {swapchain};
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    return vkQueuePresentKHR(presentQueue, &presentInfo);
}

void VulkanFrameSync::AdvanceFrame() noexcept {
    currentFrame = (currentFrame + 1) % kMaxFramesInFlight;
}

}  // namespace Spark
