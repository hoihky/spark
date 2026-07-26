#pragma once

#include "spark/core/Array.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Per-flight fences/semaphores and per-swapchain-image present sync.
 * Owns acquire → submit → present synchronization primitives used by <c>VulkanRenderer</c>.
 */
class VulkanFrameSync {
public:
    static constexpr std::uint32_t kMaxFramesInFlight = 2;

    void Create(VkDevice device);
    void DestroyFlightSync(VkDevice device) noexcept;

    void RecreateSwapchainSync(VkDevice device, std::size_t swapchainImageCount);
    void DestroySwapchainSync(VkDevice device) noexcept;

    void WaitForCurrentFrameFence(VkDevice device) const;
    [[nodiscard]] VkResult AcquireNextImage(
            VkDevice device,
            VkSwapchainKHR swapchain,
            std::uint32_t& outImageIndex) const;
    void WaitForSwapchainImageFence(VkDevice device, std::uint32_t imageIndex) const;
    void TrackSwapchainImageInFlight(std::uint32_t imageIndex) noexcept;

    void WaitForAllOtherFrames(VkDevice device) const;
    void ResetCurrentFrameFence(VkDevice device) const;

    void SubmitFrame(VkQueue graphicsQueue, VkCommandBuffer commandBuffer, std::uint32_t imageIndex) const;
    void WaitForSubmittedFrame(VkDevice device) const;

    [[nodiscard]] VkResult PresentFrame(
            VkQueue presentQueue,
            VkSwapchainKHR swapchain,
            std::uint32_t imageIndex) const;

    void AdvanceFrame() noexcept;

    [[nodiscard]] std::uint32_t CurrentFrameIndex() const noexcept { return currentFrame; }
    [[nodiscard]] static constexpr std::uint32_t MaxFramesInFlight() noexcept { return kMaxFramesInFlight; }
    [[nodiscard]] const VkFence* InFlightFences() const noexcept { return inFlightFences.GetData(); }

private:
    Array<VkSemaphore> imageAvailableSemaphores;
    /** One per swapchain image — indexed by acquired <c>imageIndex</c> for submit/present signal. */
    Array<VkSemaphore> renderFinishedSemaphores;
    Array<VkFence> inFlightFences;
    Array<VkFence> imagesInFlight;

    std::uint32_t currentFrame = 0;
};

}  // namespace Spark
