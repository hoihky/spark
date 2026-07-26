#pragma once

#include "spark/core/Array.hpp"
#include "spark/media/VideoRecordingSettings.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/render/core/VulkanFrameSync.hpp"

#include <vulkan/vulkan.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>

namespace Spark {

class VideoRecorder;

/**
 * Continuous swapchain readback for MP4 recording (same image as PNG screenshots).
 * One staging buffer per in-flight frame so readback can complete after the flight fence signals
 * without blocking presentation. Heavy watermark / scale / encode work runs on a worker thread.
 */
class VulkanVideoCapture {
public:
    void Create(VkPhysicalDevice physicalDevice, VkDevice device, VkFormat swapchainFormat);
    void Destroy(VkDevice device);

    void EnsureBuffer(VkExtent2D extent);

    [[nodiscard]] bool IsRecording() const noexcept;

    [[nodiscard]] VideoRecorder* GetRecorder() noexcept;
    [[nodiscard]] const VideoRecorder* GetRecorder() const noexcept;

    bool BeginRecording(UniquePtr<VideoRecorder> recorder, const VideoRecordingSettings& settings, VkExtent2D extent);
    bool EndRecording() noexcept;

    void RecordCopyFromSwapchain(
            VkCommandBuffer commandBuffer,
            VkImage swapchainImage,
            VkExtent2D extent,
            std::uint32_t flightIndex);

    /** Call after <c>flightIndex</c>'s in-flight fence has signaled (start of next use of that slot). */
    void TryCommitFlightCapture(std::uint32_t flightIndex) noexcept;

    /** Blocks until all in-flight captures finish, then commits any queued frames. */
    void FlushPendingCaptures(VkDevice device, const VkFence* inFlightFences) noexcept;

private:
    struct CaptureSlot {
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
        void* stagingMapped = nullptr;
        VkDeviceSize stagingBytes = 0;
        VkDeviceSize rowPitch = 0;
        VkExtent2D bufferExtent{};
        bool captureQueued = false;
        VkExtent2D captureExtent{};
        std::uint64_t captureSequence = 0;
        double capturePtsSeconds = 0.0;
    };

    struct EncodeJob {
        Array<std::uint8_t> pixels;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
        double ptsSeconds = 0.0;
    };

    static constexpr std::size_t kMaxPendingEncodeJobs = 2;

    void EnsureSlotBuffer(CaptureSlot& slot, VkExtent2D extent);
    void DestroySlot(VkDevice device, CaptureSlot& slot) noexcept;
    void CommitSlot(CaptureSlot& slot) noexcept;
    void ProcessEncodeJob(EncodeJob job) noexcept;
    void EnqueueEncodeJob(EncodeJob job);
    void StartEncodeWorker();
    void StopEncodeWorker();
    void DrainEncodeQueue();
    [[nodiscard]] bool EncodeBacklogHigh() const noexcept;
    [[nodiscard]] double NextPresentationTimeSeconds() noexcept;
    void EncodeWorkerLoop();
    void CreateResolveImage(std::uint32_t width, std::uint32_t height);
    void DestroyResolveImage(VkDevice device) noexcept;
    [[nodiscard]] bool ShouldCaptureGpuFrame() noexcept;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;

    CaptureSlot slots[VulkanFrameSync::kMaxFramesInFlight]{};
    std::uint64_t nextCaptureSequence = 0;
    std::uint64_t gpuCaptureTick = 0;

    VkImage resolveImage = VK_NULL_HANDLE;
    VkDeviceMemory resolveMemory = VK_NULL_HANDLE;
    VkImageLayout resolveLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    std::uint32_t resolveWidth = 0;
    std::uint32_t resolveHeight = 0;

    VideoRecordingSettings settings{};
    UniquePtr<VideoRecorder> recorder;
    std::uint64_t committedVideoFrames = 0;
    std::chrono::steady_clock::time_point recordWallStart{};
    double lastCommittedPtsSeconds = -1.0;
    bool recordWallClockValid = false;

    Array<std::uint8_t> workerScratchBgra;
    Array<std::uint8_t> workerScratchRgba;

    mutable std::mutex encodeMutex;
    std::condition_variable encodeCv;
    std::deque<EncodeJob> encodeQueue;
    std::thread encodeThread;
    std::atomic<bool> encodeStop{false};
    std::atomic<bool> encodeWorkerRunning{false};
    std::atomic<bool> encodeWorkerBusy{false};
};

}  // namespace Spark
