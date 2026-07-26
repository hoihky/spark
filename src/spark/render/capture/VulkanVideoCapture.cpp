#include "spark/render/capture/VulkanVideoCapture.hpp"

#include "spark/media/VideoRecorder.hpp"
#include "spark/render/capture/FrameCaptureWatermark.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace Spark {

namespace {

VkDeviceSize AlignUp(VkDeviceSize value, VkDeviceSize alignment) {
    if (alignment == 0) {
        return value;
    }
    return (value + alignment - 1) & ~(alignment - 1);
}

void CmdImageBarrier(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags srcStage,
        VkPipelineStageFlags dstStage,
        VkAccessFlags srcAccess,
        VkAccessFlags dstAccess) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = srcAccess;
    barrier.dstAccessMask = dstAccess;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

}  // namespace

bool VulkanVideoCapture::IsRecording() const noexcept {
    return recorder.Get() != nullptr && recorder->IsActive();
}

VideoRecorder* VulkanVideoCapture::GetRecorder() noexcept {
    return recorder.Get();
}

const VideoRecorder* VulkanVideoCapture::GetRecorder() const noexcept {
    return recorder.Get();
}

void VulkanVideoCapture::DestroySlot(VkDevice device, CaptureSlot& slot) noexcept {
    if (slot.stagingMapped != nullptr) {
        vkUnmapMemory(device, slot.stagingMemory);
        slot.stagingMapped = nullptr;
    }
    if (slot.stagingBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, slot.stagingBuffer, nullptr);
        slot.stagingBuffer = VK_NULL_HANDLE;
    }
    if (slot.stagingMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, slot.stagingMemory, nullptr);
        slot.stagingMemory = VK_NULL_HANDLE;
    }
    slot.stagingBytes = 0;
    slot.rowPitch = 0;
    slot.bufferExtent = {};
    slot.captureQueued = false;
    slot.captureExtent = {};
    slot.captureSequence = 0;
    slot.capturePtsSeconds = 0.0;
}

void VulkanVideoCapture::Create(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        const VkFormat swapchainFormat) {
    this->physicalDevice = physicalDevice;
    this->device = device;
    this->swapchainFormat = swapchainFormat;
}

void VulkanVideoCapture::Destroy(VkDevice device) {
    (void)EndRecording();
    DestroyResolveImage(device);
    for (CaptureSlot& slot : slots) {
        DestroySlot(device, slot);
    }
    this->device = VK_NULL_HANDLE;
    this->physicalDevice = VK_NULL_HANDLE;
}

void VulkanVideoCapture::DestroyResolveImage(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) {
        return;
    }
    if (resolveImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, resolveImage, nullptr);
        resolveImage = VK_NULL_HANDLE;
    }
    if (resolveMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, resolveMemory, nullptr);
        resolveMemory = VK_NULL_HANDLE;
    }
    resolveLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolveWidth = 0;
    resolveHeight = 0;
}

void VulkanVideoCapture::CreateResolveImage(const std::uint32_t width, const std::uint32_t height) {
    if (device == VK_NULL_HANDLE || width == 0 || height == 0 || swapchainFormat == VK_FORMAT_UNDEFINED) {
        return;
    }
    DestroyResolveImage(device);
    VulkanRendererGpu::CreateImage(
            physicalDevice,
            device,
            width,
            height,
            swapchainFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            resolveImage,
            resolveMemory);
    resolveWidth = width;
    resolveHeight = height;
    resolveLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

bool VulkanVideoCapture::ShouldCaptureGpuFrame() noexcept {
    const std::uint32_t recordFps = settings.fps > 0 ? settings.fps : 60U;
    if (recordFps >= 60U) {
        return true;
    }
    const std::uint32_t captureEvery = (60U + recordFps - 1U) / recordFps;
    if (captureEvery <= 1U) {
        return true;
    }
    return (gpuCaptureTick++ % captureEvery) == 0U;
}

void VulkanVideoCapture::EnsureSlotBuffer(CaptureSlot& slot, const VkExtent2D extent) {
    if (device == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        return;
    }
    if (slot.bufferExtent.width == extent.width && slot.bufferExtent.height == extent.height &&
        slot.stagingBuffer != VK_NULL_HANDLE) {
        return;
    }

    DestroySlot(device, slot);

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(physicalDevice, &props);
    const VkDeviceSize alignment = props.limits.optimalBufferCopyRowPitchAlignment;
    slot.rowPitch = AlignUp(static_cast<VkDeviceSize>(extent.width) * 4U, alignment);
    slot.stagingBytes = slot.rowPitch * static_cast<VkDeviceSize>(extent.height);

    VulkanRendererGpu::CreateBuffer(
            physicalDevice,
            device,
            slot.stagingBytes,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            slot.stagingBuffer,
            slot.stagingMemory);
    if (vkMapMemory(device, slot.stagingMemory, 0, slot.stagingBytes, 0, &slot.stagingMapped) != VK_SUCCESS) {
        throw std::runtime_error("VulkanVideoCapture: vkMapMemory failed");
    }
    slot.bufferExtent = extent;
}

void VulkanVideoCapture::EnsureBuffer(const VkExtent2D extent) {
    for (CaptureSlot& slot : slots) {
        EnsureSlotBuffer(slot, extent);
    }
}

void VulkanVideoCapture::StartEncodeWorker() {
    if (encodeWorkerRunning.load(std::memory_order_acquire)) {
        return;
    }
    encodeStop.store(false, std::memory_order_release);
    encodeWorkerRunning.store(true, std::memory_order_release);
    encodeThread = std::thread([this]() { EncodeWorkerLoop(); });
}

void VulkanVideoCapture::StopEncodeWorker() {
    if (!encodeWorkerRunning.load(std::memory_order_acquire)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(encodeMutex);
        encodeStop.store(true, std::memory_order_release);
    }
    encodeCv.notify_all();
    if (encodeThread.joinable()) {
        encodeThread.join();
    }
    encodeWorkerRunning.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(encodeMutex);
        encodeQueue.clear();
    }
}

void VulkanVideoCapture::DrainEncodeQueue() {
    if (!encodeWorkerRunning.load(std::memory_order_acquire)) {
        return;
    }
    std::unique_lock<std::mutex> lock(encodeMutex);
    encodeCv.wait(lock, [this]() {
        return encodeQueue.empty() && !encodeWorkerBusy.load(std::memory_order_acquire);
    });
}

bool VulkanVideoCapture::EncodeBacklogHigh() const noexcept {
    std::lock_guard<std::mutex> lock(encodeMutex);
    return encodeQueue.size() >= kMaxPendingEncodeJobs;
}

bool VulkanVideoCapture::BeginRecording(
        UniquePtr<VideoRecorder> recorder,
        const VideoRecordingSettings& settings,
        const VkExtent2D extent) {
    (void)EndRecording();
    if (!recorder || extent.width == 0 || extent.height == 0) {
        return false;
    }
    if (!recorder->Begin(settings, extent.width, extent.height)) {
        return false;
    }
    this->recorder = MoveTemp(recorder);
    this->settings = settings;
    committedVideoFrames = 0;
    nextCaptureSequence = 0;
    gpuCaptureTick = 0;
    recordWallStart = std::chrono::steady_clock::now();
    lastCommittedPtsSeconds = -1.0;
    recordWallClockValid = true;

    resolveWidth = this->recorder->OutputWidth();
    resolveHeight = this->recorder->OutputHeight();
    CreateResolveImage(resolveWidth, resolveHeight);

    const VkExtent2D stagingExtent{resolveWidth, resolveHeight};
    EnsureBuffer(stagingExtent);
    StartEncodeWorker();
    return true;
}

bool VulkanVideoCapture::EndRecording() noexcept {
    for (CaptureSlot& slot : slots) {
        slot.captureQueued = false;
        slot.captureExtent = {};
    }

    DrainEncodeQueue();
    StopEncodeWorker();

    DestroyResolveImage(device);

    if (!recorder) {
        return false;
    }
    const bool ok = recorder->End();
    recorder.Reset();
    settings = {};
    committedVideoFrames = 0;
    nextCaptureSequence = 0;
    recordWallClockValid = false;
    lastCommittedPtsSeconds = -1.0;
    workerScratchBgra.Clear();
    workerScratchRgba.Clear();
    return ok;
}

void VulkanVideoCapture::RecordCopyFromSwapchain(
        VkCommandBuffer commandBuffer,
        VkImage swapchainImage,
        const VkExtent2D extent,
        const std::uint32_t flightIndex) {
    if (!IsRecording() || swapchainImage == VK_NULL_HANDLE || extent.width == 0 || extent.height == 0) {
        return;
    }
    if (flightIndex >= VulkanFrameSync::kMaxFramesInFlight) {
        return;
    }
    if (EncodeBacklogHigh() || !ShouldCaptureGpuFrame()) {
        return;
    }
    if (resolveImage == VK_NULL_HANDLE || resolveWidth == 0 || resolveHeight == 0) {
        return;
    }

    CaptureSlot& slot = slots[flightIndex];
    const VkExtent2D stagingExtent{resolveWidth, resolveHeight};
    EnsureSlotBuffer(slot, stagingExtent);
    slot.captureExtent = stagingExtent;
    slot.captureQueued = true;
    slot.captureSequence = ++nextCaptureSequence;
    slot.capturePtsSeconds = NextPresentationTimeSeconds();

    CmdImageBarrier(
            commandBuffer,
            swapchainImage,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            VK_ACCESS_TRANSFER_READ_BIT);

    const VkImageLayout resolveOldLayout = resolveLayout;
    CmdImageBarrier(
            commandBuffer,
            resolveImage,
            resolveOldLayout,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            VK_ACCESS_TRANSFER_WRITE_BIT);

    VkImageBlit blit{};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.layerCount = 1;
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.layerCount = 1;
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {static_cast<std::int32_t>(extent.width), static_cast<std::int32_t>(extent.height), 1};
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {static_cast<std::int32_t>(resolveWidth), static_cast<std::int32_t>(resolveHeight), 1};
    vkCmdBlitImage(
            commandBuffer,
            swapchainImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            resolveImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &blit,
            VK_FILTER_LINEAR);

    CmdImageBarrier(
            commandBuffer,
            swapchainImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            0);

    CmdImageBarrier(
            commandBuffer,
            resolveImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT);
    resolveLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = resolveWidth;
    region.imageExtent.height = resolveHeight;
    region.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(
            commandBuffer,
            resolveImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            slot.stagingBuffer,
            1,
            &region);
}

double VulkanVideoCapture::NextPresentationTimeSeconds() noexcept {
    if (!recordWallClockValid) {
        return 0.0;
    }
    const auto now = std::chrono::steady_clock::now();
    double pts = std::chrono::duration<double>(now - recordWallStart).count();
    constexpr double kMinPtsStepSeconds = 1.0 / 600.0;
    if (pts <= lastCommittedPtsSeconds) {
        pts = lastCommittedPtsSeconds + kMinPtsStepSeconds;
    }
    lastCommittedPtsSeconds = pts;
    ++committedVideoFrames;
    return pts;
}

void VulkanVideoCapture::EnqueueEncodeJob(EncodeJob job) {
    {
        std::lock_guard<std::mutex> lock(encodeMutex);
        encodeQueue.push_back(std::move(job));
    }
    encodeCv.notify_one();
}

void VulkanVideoCapture::CommitSlot(CaptureSlot& slot) noexcept {
    if (!slot.captureQueued || !IsRecording() || slot.stagingMapped == nullptr || slot.captureExtent.width == 0 ||
        slot.captureExtent.height == 0) {
        slot.captureQueued = false;
        return;
    }

    if (device != VK_NULL_HANDLE && slot.stagingMemory != VK_NULL_HANDLE) {
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = slot.stagingMemory;
        range.size = slot.stagingBytes;
        (void)vkInvalidateMappedMemoryRanges(device, 1, &range);
    }

    const std::uint32_t width = slot.captureExtent.width;
    const std::uint32_t height = slot.captureExtent.height;
    const std::size_t tightBgraCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;

    EncodeJob job{};
    job.width = width;
    job.height = height;
    job.ptsSeconds = slot.capturePtsSeconds;
    job.pixels.Resize(tightBgraCount);

    const auto* src = static_cast<const std::uint8_t*>(slot.stagingMapped);
    const std::size_t tightRowBytes = static_cast<std::size_t>(width) * 4U;
    if (static_cast<VkDeviceSize>(tightRowBytes) == slot.rowPitch) {
        std::memcpy(job.pixels.GetData(), src, tightBgraCount);
    } else {
        for (std::uint32_t y = 0; y < height; ++y) {
            const std::uint8_t* row = src + static_cast<std::size_t>(y) * static_cast<std::size_t>(slot.rowPitch);
            std::memcpy(job.pixels.GetData() + static_cast<std::size_t>(y) * tightRowBytes, row, tightRowBytes);
        }
    }

    slot.captureQueued = false;
    slot.captureExtent = {};
    slot.captureSequence = 0;
    slot.capturePtsSeconds = 0.0;

    EnqueueEncodeJob(std::move(job));
}

void VulkanVideoCapture::ProcessEncodeJob(EncodeJob job) noexcept {
    encodeWorkerBusy.store(true, std::memory_order_release);
    if (!IsRecording() || job.pixels.IsEmpty() || job.width == 0 || job.height == 0) {
        encodeWorkerBusy.store(false, std::memory_order_release);
        encodeCv.notify_all();
        return;
    }

    workerScratchBgra = std::move(job.pixels);

    const std::uint32_t width = job.width;
    const std::uint32_t height = job.height;
    const std::size_t tightBgraCount = workerScratchBgra.GetSize();

    if (settings.applyWatermark) {
        workerScratchRgba.Resize(tightBgraCount);
        ConvertBgraRowsToRgba(
                workerScratchBgra.GetData(),
                width,
                height,
                static_cast<std::size_t>(width) * 4U,
                workerScratchRgba.GetData());
        ApplyFrameCaptureWatermark(workerScratchRgba.GetData(), width, height);
        ConvertRgbaToBgraRows(
                workerScratchRgba.GetData(),
                width,
                height,
                workerScratchBgra.GetData(),
                static_cast<std::size_t>(width) * 4U);
    }

    const std::uint32_t outW = recorder->OutputWidth();
    const std::uint32_t outH = recorder->OutputHeight();
    if (outW != width || outH != height) {
        const std::size_t scaledCount = static_cast<std::size_t>(outW) * static_cast<std::size_t>(outH) * 4U;
        workerScratchRgba.Resize(scaledCount);
        DownscaleBgraBox(workerScratchBgra.GetData(), width, height, workerScratchRgba.GetData(), outW, outH);
        recorder->AppendVideoFrameBgra(workerScratchRgba.GetData(), outW, outH, job.ptsSeconds);
    } else {
        recorder->AppendVideoFrameBgra(workerScratchBgra.GetData(), width, height, job.ptsSeconds);
    }

    encodeWorkerBusy.store(false, std::memory_order_release);
    encodeCv.notify_all();
}

void VulkanVideoCapture::EncodeWorkerLoop() {
    while (true) {
        EncodeJob job;
        {
            std::unique_lock<std::mutex> lock(encodeMutex);
            encodeCv.wait(lock, [this]() {
                return encodeStop.load(std::memory_order_acquire) || !encodeQueue.empty();
            });
            if (encodeStop.load(std::memory_order_acquire) && encodeQueue.empty()) {
                break;
            }
            job = std::move(encodeQueue.front());
            encodeQueue.pop_front();
            encodeCv.notify_all();
        }
        ProcessEncodeJob(std::move(job));
    }
}

void VulkanVideoCapture::TryCommitFlightCapture(const std::uint32_t flightIndex) noexcept {
    if (flightIndex >= VulkanFrameSync::kMaxFramesInFlight) {
        return;
    }
    CommitSlot(slots[flightIndex]);
}

void VulkanVideoCapture::FlushPendingCaptures(VkDevice device, const VkFence* inFlightFences) noexcept {
    if (!IsRecording() || inFlightFences == nullptr) {
        return;
    }

    std::uint32_t pendingFlights[VulkanFrameSync::kMaxFramesInFlight]{};
    std::uint64_t pendingSequences[VulkanFrameSync::kMaxFramesInFlight]{};
    std::uint32_t pendingCount = 0;
    for (std::uint32_t i = 0; i < VulkanFrameSync::kMaxFramesInFlight; ++i) {
        if (!slots[i].captureQueued) {
            continue;
        }
        pendingFlights[pendingCount] = i;
        pendingSequences[pendingCount] = slots[i].captureSequence;
        ++pendingCount;
    }
    if (pendingCount == 0) {
        return;
    }

    for (std::uint32_t a = 0; a + 1 < pendingCount; ++a) {
        for (std::uint32_t b = a + 1; b < pendingCount; ++b) {
            if (pendingSequences[b] < pendingSequences[a]) {
                const std::uint32_t tmpFlight = pendingFlights[a];
                pendingFlights[a] = pendingFlights[b];
                pendingFlights[b] = tmpFlight;
                const std::uint64_t tmpSeq = pendingSequences[a];
                pendingSequences[a] = pendingSequences[b];
                pendingSequences[b] = tmpSeq;
            }
        }
    }

    for (std::uint32_t i = 0; i < pendingCount; ++i) {
        const std::uint32_t flight = pendingFlights[i];
        vkWaitForFences(device, 1, &inFlightFences[flight], VK_TRUE, UINT64_MAX);
        TryCommitFlightCapture(flight);
    }

    DrainEncodeQueue();
}

}  // namespace Spark
