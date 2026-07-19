#include "spark/render/core/VulkanRenderer.hpp"

#include "spark/config.hpp"
#include "spark/render/scene/SceneGroundExtent.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/Mesh.hpp"
#include "spark/scene/SkinnedMesh.hpp"
#include "spark/scene/Texture2D.hpp"
#include "spark/media/VideoRecorder.hpp"
#include "spark/render/platform/Window.hpp"

#include <GLFW/glfw3.h>

#include <chrono>

#include "spark/render/lighting/SceneLightingResolver.hpp"
#include "spark/render/core/VulkanRendererGpu.hpp"
#include "spark/render/scene/VulkanSceneVertexLayout.hpp"
#include "spark/render/post/VulkanScreenSpaceEffectsPass.hpp"

#include "spark/config.hpp"
#if SPARK_ENABLE_IMGUI
#include "spark/imgui/ImGuiVulkanBackend.hpp"
#include "spark/imgui/ImGuiVulkanBackendAccess.hpp"
#endif

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace Spark {

VulkanRenderer::VulkanRenderer(Window& inAppWindow) : deviceContext(inAppWindow), boundWindow(&inAppWindow) {
    shaderLoader.SetDevice(device());

    CreateCommandPool();
    CreatePersistentSceneResources();
    RecreateSwapchain();
    frameSync.Create(device());

    imguiLayer = CreateImGuiLayer();
    SetActiveImGuiLayer(imguiLayer.Get());
    InitImGuiBackend();
}

VulkanRenderer::~VulkanRenderer() {
    if (device() == VK_NULL_HANDLE) {
        return;
    }
    deviceContext.WaitDeviceIdle();

    ShutdownImGuiBackend();
    SetActiveImGuiLayer(nullptr);
    imguiLayer.Reset();

    DestroyPersistentSceneResources();

    frameSync.DestroyFlightSync(device());

    vkDestroyCommandPool(device(), commandPool, nullptr);

    CleanupSwapchain();
}

void VulkanRenderer::CleanupSwapchain() {
    InvalidateImGuiBackend();
    frameSync.DestroySwapchainSync(device());
    screenSpaceEffectsPass.DestroyPipeline(device());
    screenSpaceEffectsPass.DestroyFlightTargets(device());
    screenSpaceEffectsPass.DestroyRenderPass(device());
    hdrTonemapPass.DestroyTonemapPipeline(device());
    hdrTonemapPass.DestroyFlightTargets(device());
    hdrTonemapPass.DestroyRenderPass(device());
    DestroyDepthResources();

    presentationFramebuffers.Destroy(device());

    scenePipeline.DestroyGraphicsPipeline(device());
    screenUi.DestroyPipelines(device());
    particlePass.DestroyGraphicsPipeline(device());
    tilemapPass.DestroyGraphicsPipeline(device());
    spritePass.DestroyGraphicsPipeline(device());
    presentRenderPass.Destroy(device());
    deviceContext.DestroySwapchain();
}

void VulkanRenderer::RecreateSwapchain() {
    deviceContext.WaitDeviceIdle();

    if (!commandBuffers.IsEmpty()) {
        vkFreeCommandBuffers(
                device(),
                commandPool,
                static_cast<std::uint32_t>(commandBuffers.GetSize()),
                commandBuffers.GetData());
        commandBuffers.Clear();
    }

    CleanupSwapchain();

    deviceContext.RecreateSwapchain();

    frameCapture.RecreateSwapchainResources(
            physicalDevice(), device(), presentSwapchain().imageFormat, presentSwapchain().extent);

    sceneDepthFormat = VulkanRendererGpu::FindDepthFormat(physicalDevice());
    CreateDepthResources();
    CreateRenderPass();
    screenSpaceEffectsPass.CreateRenderPass(device());
    RecreateHdrFlightTargets();
    screenSpaceEffectsPass.RecreateFlightTargets(
            physicalDevice(), device(), presentSwapchain().extent, sceneDepthFormat, VulkanFrameSync::kMaxFramesInFlight);
    scenePipeline.CreateGraphicsPipeline(
            device(), hdrTonemapPass.HdrRenderPass(), sceneDescriptors.Layout(), shaderLoader);
    screenUi.CreatePipelines(device(), presentRenderPass.vkPass);
    particlePass.CreateGraphicsPipeline(device(), hdrTonemapPass.HdrRenderPass());
    tilemapPass.CreateGraphicsPipeline(
            device(), hdrTonemapPass.HdrRenderPass(), sceneDescriptors.Layout(), shaderLoader);
    spritePass.CreateGraphicsPipeline(
            device(), hdrTonemapPass.HdrRenderPass(), sceneDescriptors.Layout(), shaderLoader);
    CreateFramebuffers();
    hdrTonemapPass.CreateTonemapPipeline(
            physicalDevice(), device(), presentRenderPass.vkPass, VulkanFrameSync::kMaxFramesInFlight, shaderLoader);
    screenSpaceEffectsPass.CreatePipeline(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight, shaderLoader);
    for (std::uint32_t fi = 0; fi < VulkanFrameSync::kMaxFramesInFlight; ++fi) {
        if (fi < sceneDescriptors.DescriptorSetCount() && hdrTonemapPass.HasFlight(fi) &&
            fi < sceneDepthResources.GetSize()) {
            screenSpaceEffectsPass.UpdateDescriptor(
                    device(),
                    fi,
                    sceneDescriptors.UniformBuffer(static_cast<std::uint32_t>(fi)),
                    hdrTonemapPass.Flight(fi),
                    screenSpaceEffectsPass.Flight(fi));
        }
    }

    commandBuffers.Resize(presentationFramebuffers.buffers.GetSize());
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = commandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = static_cast<std::uint32_t>(commandBuffers.GetSize());
    if (vkAllocateCommandBuffers(device(), &commandBufferAllocateInfo, commandBuffers.GetData()) != VK_SUCCESS) {
        throw std::runtime_error("vkAllocateCommandBuffers failed");
    }

    frameSync.RecreateSwapchainSync(device(), presentSwapchain().images.GetSize());
    InitImGuiBackend();
}

void VulkanRenderer::CreateRenderPass() {
    hdrTonemapPass.CreateRenderPass(device(), sceneDepthFormat);
    presentRenderPass.Create(device(), presentSwapchain().imageFormat, VK_FORMAT_UNDEFINED);
}

void VulkanRenderer::RecreateHdrFlightTargets() {
    Array<VkImageView> depthViews;
    depthViews.Resize(VulkanFrameSync::kMaxFramesInFlight);
    for (std::size_t fi = 0; fi < VulkanFrameSync::kMaxFramesInFlight; ++fi) {
        depthViews[fi] = sceneDepthResources[fi].view;
    }
    hdrTonemapPass.RecreateFlightTargets(
            physicalDevice(),
            device(),
            presentSwapchain().extent,
            VulkanFrameSync::kMaxFramesInFlight,
            depthViews.GetData(),
            depthViews.GetSize());
}


void VulkanRenderer::CreateFramebuffers() {
    presentationFramebuffers.Create(
            device(),
            presentRenderPass.vkPass,
            presentSwapchain().extent,
            presentSwapchain().imageViews,
            VK_NULL_HANDLE);
}

void VulkanRenderer::CreateCommandPool() {
    const VulkanRendererGpu::QueueFamilyIndices& queueFamilies = deviceContext.GetQueueFamilies();
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamily;
    if (vkCreateCommandPool(device(), &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateCommandPool failed");
    }
}

void VulkanRenderer::RecordSceneCommandBuffer(
        VkCommandBuffer commandBuffer,
        std::uint32_t imageIndex,
        std::uint32_t frameIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("vkBeginCommandBuffer failed");
    }

    deferredUploadBatch.Record(commandBuffer, device(), sceneTextureUploader, screenUi);
    customMeshPool.RecordUploads(commandBuffer);
    RecordShadowMapPass(commandBuffer, frameIndex);

    hdrTonemapPass.BeginColorAttachmentBarrierIfNeeded(commandBuffer, frameIndex);

    if (frameIndex < sceneDepthResources.GetSize()) {
        VulkanDepthResources& depth = sceneDepthResources[frameIndex];
        if (depth.image != VK_NULL_HANDLE && depth.imageLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
            VkImageMemoryBarrier depthBarrier{};
            depthBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            depthBarrier.srcAccessMask = 0;
            depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            depthBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            depthBarrier.image = depth.image;
            depthBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthBarrier.subresourceRange.levelCount = 1;
            depthBarrier.subresourceRange.layerCount = 1;
            vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &depthBarrier);
            depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }

    VkClearValue clearColors[2]{};
    clearColors[0].color.float32[0] = 0.0F;
    clearColors[0].color.float32[1] = 0.0F;
    clearColors[0].color.float32[2] = 0.0F;
    clearColors[0].color.float32[3] = 1.0F;
    clearColors[1].depthStencil.depth = 1.0F;
    clearColors[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    if (!hdrTonemapPass.HasFlight(frameIndex) ||
        hdrTonemapPass.Flight(frameIndex).framebuffer == VK_NULL_HANDLE) {
        throw std::runtime_error("RecordSceneCommandBuffer: HDR framebuffer missing");
    }

    renderPassBeginInfo.renderPass = hdrTonemapPass.HdrRenderPass();
    renderPassBeginInfo.framebuffer = hdrTonemapPass.Flight(frameIndex).framebuffer;
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = presentSwapchain().extent;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearColors;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (frameIndex < sceneDescriptors.DescriptorSetCount()) {
        const VulkanCustomMeshPool::Bindings customMesh = customMeshPool.GetBindings();
        const VulkanSceneOpaqueRecordContext opaqueCtx{
                .scene = &pendingScene,
                .sceneParamsValid = sceneParamsValid,
                .frameIndex = frameIndex,
                .extent = presentSwapchain().extent,
                .pipelineLit = scenePipeline.PipelineLit(),
                .pipelineSky = scenePipeline.PipelineSky(),
                .pipelineLitTransparent = scenePipeline.PipelineLitTransparent(),
                .pipelineLayout = scenePipeline.PipelineLayout(),
                .descriptorSet = sceneDescriptors.DescriptorSet(frameIndex),
                .meshBindings =
                        {
                                .staticVertexBuffer = vertexBuffer,
                                .staticIndexBuffer = indexBuffer,
                                .customVertexBuffer = customMesh.vertexBuffer,
                                .customIndexBuffer = customMesh.indexBuffer,
                                .cubeIndexCount = cubeIndexCount,
                                .planeIndexCount = planeIndexCount,
                                .planeFirstIndex = planeFirstIndex,
                                .cubeVertexOffset = cubeVertexOffset,
                                .planeVertexOffset = planeVertexOffset,
                                .customDrawPacked = &customDrawPacked,
                        },
                .skinSsboMapped = &sceneDescriptors.SkinSsboMapped(),
                .maxSkinJoints = VulkanSceneDescriptors::kMaxSkinJoints,
        };
        sceneOpaquePass.Record(commandBuffer, opaqueCtx);
        sceneOpaquePass.RecordTransparent(commandBuffer, opaqueCtx, customDrawPackedTransparent);
    }

    if (sceneParamsValid && frameIndex < sceneDescriptors.DescriptorSetCount()) {
        const VulkanTilemapRecordContext tilemapCtx{
                .scene = &pendingScene,
                .sceneParamsValid = sceneParamsValid,
                .frameIndex = frameIndex,
                .extent = presentSwapchain().extent,
                .vertexBuffer = vertexBuffer,
                .indexBuffer = indexBuffer,
                .quadFirstIndex = spriteQuadFirstIndex,
                .quadIndexCount = spriteQuadIndexCount,
                .descriptorSet = sceneDescriptors.DescriptorSet(frameIndex),
        };
        const VulkanSpriteRecordContext spriteCtx{
                .scene = &pendingScene,
                .sceneParamsValid = sceneParamsValid,
                .frameIndex = frameIndex,
                .extent = presentSwapchain().extent,
                .vertexBuffer = vertexBuffer,
                .indexBuffer = indexBuffer,
                .quadFirstIndex = spriteQuadFirstIndex,
                .quadIndexCount = spriteQuadIndexCount,
                .descriptorSet = sceneDescriptors.DescriptorSet(frameIndex),
        };
        composite2DPass.Record(commandBuffer, tilemapPass, spritePass, tilemapCtx, spriteCtx);
    }
    particlePass.Record(commandBuffer, frameIndex, presentSwapchain().extent, pendingScene, sceneParamsValid);

    vkCmdEndRenderPass(commandBuffer);

    hdrTonemapPass.TransitionColorToShaderRead(commandBuffer, frameIndex);

    const bool ssaoActive = sceneParamsValid && pendingScene.ssaoEnabled;
    if (ssaoActive && frameIndex < sceneDescriptors.DescriptorSetCount() && hdrTonemapPass.HasFlight(frameIndex) &&
        screenSpaceEffectsPass.HasFlight(frameIndex)) {
        screenSpaceEffectsPass.UpdateDescriptor(
                device(),
                frameIndex,
                sceneDescriptors.UniformBuffer(frameIndex),
                hdrTonemapPass.Flight(frameIndex),
                screenSpaceEffectsPass.Flight(frameIndex));
        screenSpaceEffectsPass.Record(
                commandBuffer,
                frameIndex,
                presentSwapchain().extent,
                sceneDepthResources[frameIndex],
                pendingScene);
        hdrTonemapPass.UpdateTonemapDescriptor(
                device(), frameIndex, screenSpaceEffectsPass.OutputView(frameIndex));
    } else {
        hdrTonemapPass.UpdateTonemapDescriptor(device(), frameIndex);
    }

    hdrTonemapPass.RecordTonemap(
            commandBuffer,
            imageIndex,
            frameIndex,
            presentRenderPass.vkPass,
            presentSwapchain().extent,
            presentationFramebuffers,
            resolvedLighting.exposure);
    screenUi.Record(
            commandBuffer,
            frameIndex,
            presentSwapchain().extent,
            pendingScene,
            sceneParamsValid);
    RecordImGuiDrawData(commandBuffer);

    vkCmdEndRenderPass(commandBuffer);

    if (imageIndex < presentSwapchain().images.GetSize()) {
        frameCapture.RecordCopyFromSwapchain(
                commandBuffer,
                presentSwapchain().images[imageIndex],
                presentSwapchain().extent);
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("vkEndCommandBuffer failed");
    }
}

void VulkanRenderer::PresentFrame() {
    DrawFrame();
}

bool VulkanRenderer::TryGetDrawableSize(int& outWidth, int& outHeight) const {
    return deviceContext.TryGetDrawableSize(outWidth, outHeight);
}

void VulkanRenderer::NotifySwapchainResize() {
    framebufferResized = true;
}

void VulkanRenderer::DrawFrame() {
    if (framebufferResized) {
        framebufferResized = false;
        RecreateSwapchain();
        return;
    }

    frameSync.WaitForCurrentFrameFence(device());
    customMeshPool.ReleaseRetiredBuffers(submittedFrameCounter);
    screenUi.ReleaseRetiredFontAtlases(device(), submittedFrameCounter);
    screenUi.ReleaseRetiredUiTextureAtlases(device(), submittedFrameCounter);

    std::uint32_t imageIndex = 0;
    const VkResult acquireResult =
            frameSync.AcquireNextImage(device(), presentSwapchain().khr, imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("vkAcquireNextImageKHR failed");
    }

    frameSync.WaitForSwapchainImageFence(device(), imageIndex);
    frameSync.TrackSwapchainImageInFlight(imageIndex);

    if (deferredUploadBatch.NeedsSceneTextureGpuIdle(sceneTextureUploader, pendingScene, sceneParamsValid) ||
        deferredUploadBatch.NeedsUiTextureGpuIdle(screenUi, pendingScene)) {
        frameSync.WaitForAllOtherFrames(device());
    }

    deferredUploadBatch.Prepare(
            sceneTextureUploader,
            screenUi,
            physicalDevice(),
            device(),
            pendingScene,
            sceneParamsValid,
            submittedFrameCounter,
            VulkanFrameSync::kMaxFramesInFlight);

    customMeshPool.UpdateFromScene(pendingScene, submittedFrameCounter, VulkanFrameSync::kMaxFramesInFlight);
    customMeshPool.FillCustomDrawPacked(pendingScene, customDrawPacked, customDrawPackedTransparent);
    ++submittedFrameCounter;

    frameSync.ResetCurrentFrameFence(device());

    const std::uint32_t currentFrame = frameSync.CurrentFrameIndex();
    WriteUniformBuffer(currentFrame);

    if (vkResetCommandBuffer(commandBuffers[imageIndex], 0) != VK_SUCCESS) {
        throw std::runtime_error("vkResetCommandBuffer failed");
    }
    RecordSceneCommandBuffer(commandBuffers[imageIndex], imageIndex, currentFrame);

    frameSync.SubmitFrame(graphicsQueue(), commandBuffers[imageIndex], imageIndex);

    if (frameCapture.NeedsPostSubmitWork()) {
        frameSync.WaitForSubmittedFrame(device());
        frameCapture.TrySavePendingPng();
        if (frameCapture.IsVideoRecording()) {
            double ptsSeconds = 0.0;
            if (recordingWallClockValid) {
                const auto now = std::chrono::steady_clock::now();
                ptsSeconds = std::chrono::duration<double>(now - recordingWallStart).count();
            }
            frameCapture.TryCommitFrameAfterFence(ptsSeconds);
        }
    }

    const VkResult presentResult =
            frameSync.PresentFrame(presentQueue(), presentSwapchain().khr, imageIndex);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        RecreateSwapchain();
    } else if (presentResult != VK_SUCCESS) {
        throw std::runtime_error("vkQueuePresentKHR failed");
    }

    frameSync.AdvanceFrame();
}

void VulkanRenderer::RequestScreenshotSave(const char* pathUtf8) {
    frameCapture.RequestScreenshotSave(pathUtf8);
}

bool VulkanRenderer::BeginVideoRecording(const VideoRecordingSettings& settings) {
    if (frameCapture.IsVideoRecording()) {
        return false;
    }
    auto recorder = VideoRecorder::Create();
    if (!recorder) {
        return false;
    }
    const VkExtent2D extent = presentSwapchain().extent;
    if (!frameCapture.BeginVideoRecording(MoveTemp(recorder), settings, extent)) {
        return false;
    }
    recordingWallStart = std::chrono::steady_clock::now();
    recordingWallClockValid = true;
    return true;
}

void VulkanRenderer::EndVideoRecording() {
    frameCapture.EndVideoRecording();
    recordingWallClockValid = false;
}

bool VulkanRenderer::IsVideoRecording() const {
    return frameCapture.IsVideoRecording();
}

VideoRecorder* VulkanRenderer::GetActiveVideoRecorder() {
    return frameCapture.GetRecorder();
}

void VulkanRenderer::SetSceneRenderParams(const SceneRenderParams& params) {
    pendingScene = params;
    sceneParamsValid = true;
    resolvedLighting = SceneLightingResolver::Resolve(pendingScene);
}

void VulkanRenderer::CreateDepthResources() {
    for (std::size_t i = 0; i < sceneDepthResources.GetSize(); ++i) {
        sceneDepthResources[i].Destroy(device());
    }
    sceneDepthResources.Resize(VulkanFrameSync::kMaxFramesInFlight);
    for (std::size_t i = 0; i < VulkanFrameSync::kMaxFramesInFlight; ++i) {
        sceneDepthResources[i].Create(device(), physicalDevice(), presentSwapchain().extent, sceneDepthFormat);
    }
}

void VulkanRenderer::DestroyDepthResources() {
    for (std::size_t i = 0; i < sceneDepthResources.GetSize(); ++i) {
        sceneDepthResources[i].Destroy(device());
    }
    sceneDepthResources.Clear();
}

void VulkanRenderer::DestroyPersistentSceneResources() {
    if (device() == VK_NULL_HANDLE) {
        return;
    }
    directionalShadow.DestroyGraphicsPipeline(device());
    punctualShadow.DestroyGraphicsPipeline(device());
    sceneDescriptors.Destroy(device());
    clusteredForwardLights.DestroyBuffers(device());
    frameCapture.Destroy(device());

    sceneTextureUploader.DestroyResources(device());
    customMeshPool.DestroyResources(device());
    directionalShadow.DestroyResources(device());
    punctualShadow.DestroyResources(device());

    customDrawPacked.Clear();
    customDrawPackedTransparent.Clear();

    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device(), vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device(), vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device(), indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if (indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device(), indexBufferMemory, nullptr);
        indexBufferMemory = VK_NULL_HANDLE;
    }

    particlePass.DestroyGraphicsPipeline(device());
    tilemapPass.DestroyGraphicsPipeline(device());
    spritePass.DestroyGraphicsPipeline(device());
    screenUi.DestroyPipelines(device());
    particlePass.DestroyGpuResources(device());
    spritePass.DestroyGpuResources(device());
    screenUi.DestroyResources(device());
}


void VulkanRenderer::CreatePersistentSceneResources() {
    sceneDescriptors.CreateSetLayout(device());
    sceneTextureUploader.CreateResources(physicalDevice(), device(), commandPool, graphicsQueue());
    customMeshPool.CreateResources(physicalDevice(), device());
    directionalShadow.CreateResources(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight);
    directionalShadow.CreateGraphicsPipeline(device(), sceneDescriptors.Layout(), shaderLoader);
    punctualShadow.CreateResources(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight);
    punctualShadow.CreateGraphicsPipeline(device(), sceneDescriptors.Layout(), shaderLoader);
    sceneDescriptors.CreateUniformBuffers(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight);
    sceneDescriptors.CreateSkinSsboBuffers(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight);
    clusteredForwardLights.CreateBuffers(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight);
    spritePass.CreateGpuResources(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight);
    sceneDescriptors.CreatePoolAndSets(
            device(),
            VulkanFrameSync::kMaxFramesInFlight,
            VulkanSceneDescriptors::BindingSources{
                    .sceneTextureUploader = sceneTextureUploader,
                    .clusteredForwardLights = clusteredForwardLights,
                    .directionalShadow = directionalShadow,
                    .punctualShadow = punctualShadow,
                    .spritePass = spritePass,
            });
    CreateSceneGeometry();
    screenUi.CreateResources(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight, shaderLoader);
    particlePass.CreateGpuResources(physicalDevice(), device(), VulkanFrameSync::kMaxFramesInFlight, shaderLoader);
}

void VulkanRenderer::RecordShadowMapPass(VkCommandBuffer commandBuffer, std::uint32_t frameIndex) {
    if (frameIndex >= sceneDescriptors.DescriptorSetCount()) {
        return;
    }
    const VulkanCustomMeshPool::Bindings customMesh = customMeshPool.GetBindings();
    VulkanShadowRecordContext ctx{};
    ctx.scene = &pendingScene;
    ctx.sceneParamsValid = sceneParamsValid;
    ctx.vertexBuffer = vertexBuffer;
    ctx.indexBuffer = indexBuffer;
    ctx.customVertexBuffer = customMesh.vertexBuffer;
    ctx.customIndexBuffer = customMesh.indexBuffer;
    ctx.cubeIndexCount = cubeIndexCount;
    ctx.planeIndexCount = planeIndexCount;
    ctx.planeFirstIndex = planeFirstIndex;
    ctx.cubeVertexOffset = cubeVertexOffset;
    ctx.planeVertexOffset = planeVertexOffset;
    ctx.customDrawPacked = &customDrawPacked;
    ctx.skinSsboMapped = &sceneDescriptors.SkinSsboMapped();
    ctx.descriptorSet = sceneDescriptors.DescriptorSet(frameIndex);
    ctx.maxSkinJoints = VulkanSceneDescriptors::kMaxSkinJoints;
    punctualShadow.Record(commandBuffer, frameIndex, ctx, punctualShadowFrameState);
    directionalShadow.Record(commandBuffer, frameIndex, ctx, shadowFrameState);
}

void VulkanRenderer::WriteUniformBuffer(std::uint32_t frameIndex) {
    void* const uniformMapped = sceneDescriptors.UniformMapped(frameIndex);
    if (!sceneParamsValid || uniformMapped == nullptr) {
        return;
    }
    punctualShadow.PrepareAndUpload(frameIndex, pendingScene, resolvedLighting, punctualShadowFrameState);
    clusteredForwardLights.BuildAndUpload(
            frameIndex,
            pendingScene,
            resolvedLighting,
            presentSwapchain().extent);
    sceneUniformWriter.Write(
            uniformMapped,
            pendingScene,
            resolvedLighting,
            presentSwapchain().extent,
            directionalShadow,
            frameIndex,
            shadowFrameState);
}

void VulkanRenderer::CreateSceneGeometry() {
    Mesh cubeMesh = Mesh::CreateUnitCube();
    Array<float> interleaved;
    interleaved.Reserve((cubeMesh.GetVertices().GetSize() + 8) * VulkanSceneVertexLayout::kFloatsPerVertex);
    for (std::size_t i = 0; i < cubeMesh.GetVertices().GetSize(); ++i) {
        VulkanRendererGpu::AppendRigidMeshVertexToInterleaved(cubeMesh.GetVertices()[i], interleaved);
    }

    const std::uint32_t planeVertexBase = static_cast<std::uint32_t>(cubeMesh.GetVertices().GetSize());
    const float groundExtent = kSceneGroundHalfExtent;
    const float groundUvSpan =
            (2.0F * groundExtent) / (std::max)(kSceneGroundWorldUnitsPerTextureRepeat, 1.0e-3F);
    const Mesh::Vertex planeVerts[4] = {
            {{-groundExtent, 0.0F, -groundExtent}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
            {{groundExtent, 0.0F, -groundExtent}, {0.0F, 1.0F, 0.0F}, {groundUvSpan, 0.0F}},
            {{groundExtent, 0.0F, groundExtent}, {0.0F, 1.0F, 0.0F}, {groundUvSpan, groundUvSpan}},
            {{-groundExtent, 0.0F, groundExtent}, {0.0F, 1.0F, 0.0F}, {0.0F, groundUvSpan}},
    };
    for (const Mesh::Vertex& pv : planeVerts) {
        VulkanRendererGpu::AppendRigidMeshVertexToInterleaved(pv, interleaved);
    }

    const std::uint32_t spriteVertexBase = planeVertexBase + 4;
    const Mesh::Vertex spriteVerts[4] = {
            {{-0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
            {{0.5F, -0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
            {{0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 1.0F}},
            {{-0.5F, 0.5F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F}},
    };
    for (const Mesh::Vertex& sv : spriteVerts) {
        VulkanRendererGpu::AppendRigidMeshVertexToInterleaved(sv, interleaved);
    }

    Array<std::uint32_t> indices;
    cubeIndexCount = static_cast<std::uint32_t>(cubeMesh.GetIndices().GetSize());
    for (std::size_t i = 0; i < cubeMesh.GetIndices().GetSize(); ++i) {
        indices.PushBack(cubeMesh.GetIndices()[i]);
    }
    planeFirstIndex = static_cast<std::uint32_t>(indices.GetSize());
    planeIndexCount = 6;
    // Both tris CCW when viewed from +Y so normals (0,1,0) match geometry (second tri was 0,2,3 → -Y).
    indices.PushBack(planeVertexBase);
    indices.PushBack(planeVertexBase + 1);
    indices.PushBack(planeVertexBase + 2);
    indices.PushBack(planeVertexBase);
    indices.PushBack(planeVertexBase + 3);
    indices.PushBack(planeVertexBase + 2);

    spriteQuadFirstIndex = static_cast<std::uint32_t>(indices.GetSize());
    spriteQuadIndexCount = 6;
    indices.PushBack(spriteVertexBase);
    indices.PushBack(spriteVertexBase + 1);
    indices.PushBack(spriteVertexBase + 2);
    indices.PushBack(spriteVertexBase);
    indices.PushBack(spriteVertexBase + 3);
    indices.PushBack(spriteVertexBase + 2);

    cubeVertexOffset = 0;
    // Indices already store absolute vertex indices (planeVertexBase..); vertexOffset must be 0.
    // Adding planeVertexBase here would double-offset and read past the vertex buffer (no ground plane).
    planeVertexOffset = 0;

    const VkDeviceSize vbSize = sizeof(float) * interleaved.GetSize();
    const VkDeviceSize ibSize = sizeof(std::uint32_t) * indices.GetSize();

    VkBuffer stagingVertex = VK_NULL_HANDLE;
    VkDeviceMemory stagingVertexMemory = VK_NULL_HANDLE;
    VulkanRendererGpu::CreateBuffer(
            physicalDevice(),
            device(),
            vbSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingVertex,
            stagingVertexMemory);
    void* vtxData = nullptr;
    if (vkMapMemory(device(), stagingVertexMemory, 0, vbSize, 0, &vtxData) != VK_SUCCESS) {
        throw std::runtime_error("map staging vertex failed");
    }
    std::memcpy(vtxData, interleaved.GetData(), static_cast<std::size_t>(vbSize));
    vkUnmapMemory(device(), stagingVertexMemory);

    VkBuffer stagingIndex = VK_NULL_HANDLE;
    VkDeviceMemory stagingIndexMemory = VK_NULL_HANDLE;
    VulkanRendererGpu::CreateBuffer(
            physicalDevice(),
            device(),
            ibSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingIndex,
            stagingIndexMemory);
    void* idxData = nullptr;
    if (vkMapMemory(device(), stagingIndexMemory, 0, ibSize, 0, &idxData) != VK_SUCCESS) {
        throw std::runtime_error("map staging index failed");
    }
    std::memcpy(idxData, indices.GetData(), static_cast<std::size_t>(ibSize));
    vkUnmapMemory(device(), stagingIndexMemory);

    VulkanRendererGpu::CreateBuffer(
            physicalDevice(),
            device(),
            vbSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            vertexBuffer,
            vertexBufferMemory);
    VulkanRendererGpu::CreateBuffer(
            physicalDevice(),
            device(),
            ibSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            indexBuffer,
            indexBufferMemory);

    VulkanRendererGpu::CopyBuffer(device(), commandPool, graphicsQueue(), stagingVertex, vertexBuffer, vbSize);
    VulkanRendererGpu::CopyBuffer(device(), commandPool, graphicsQueue(), stagingIndex, indexBuffer, ibSize);

    vkDestroyBuffer(device(), stagingVertex, nullptr);
    vkFreeMemory(device(), stagingVertexMemory, nullptr);
    vkDestroyBuffer(device(), stagingIndex, nullptr);
    vkFreeMemory(device(), stagingIndexMemory, nullptr);
}

void VulkanRenderer::InitImGuiBackend() {
#if SPARK_ENABLE_IMGUI
    if (!imguiLayer || boundWindow == nullptr || presentRenderPass.vkPass == VK_NULL_HANDLE) {
        return;
    }
    IImGuiVulkanBackend* backend = TryGetImGuiVulkanBackend(imguiLayer.Get());
    if (backend == nullptr) {
        return;
    }
    backend->InitGlfw(*boundWindow);
    ImGuiVulkanBackendInfo info{};
    info.instance = deviceContext.GetInstance();
    info.physicalDevice = deviceContext.GetPhysicalDevice();
    info.device = device();
    info.queueFamily = deviceContext.GetQueueFamilies().graphicsFamily;
    info.queue = graphicsQueue();
    info.renderPass = presentRenderPass.vkPass;
    info.minImageCount = 2;
    info.imageCount = static_cast<std::uint32_t>(std::max<std::size_t>(presentSwapchain().images.GetSize(), 2U));
    info.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    backend->RecreateVulkan(info);
#endif
}

void VulkanRenderer::InvalidateImGuiBackend() noexcept {
#if SPARK_ENABLE_IMGUI
    if (IImGuiVulkanBackend* backend = TryGetImGuiVulkanBackend(imguiLayer.Get())) {
        backend->InvalidateVulkan();
    }
#endif
}

void VulkanRenderer::ShutdownImGuiBackend() noexcept {
#if SPARK_ENABLE_IMGUI
    if (IImGuiVulkanBackend* backend = TryGetImGuiVulkanBackend(imguiLayer.Get())) {
        backend->Shutdown();
    }
#endif
}

void VulkanRenderer::RecordImGuiDrawData(const VkCommandBuffer commandBuffer) {
#if SPARK_ENABLE_IMGUI
    if (IImGuiVulkanBackend* backend = TryGetImGuiVulkanBackend(imguiLayer.Get())) {
        backend->RecordDrawData(commandBuffer);
    }
#endif
    (void)commandBuffer;
}

}  // namespace Spark
