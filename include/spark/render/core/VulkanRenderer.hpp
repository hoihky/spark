#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/IFramePresenter.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/media/VideoRecordingSettings.hpp"

#include <vulkan/vulkan.h>

#include "spark/render/core/VulkanDeviceContext.hpp"
#include "spark/render/core/VulkanFrameCapture.hpp"
#include "spark/render/core/VulkanFrameSync.hpp"
#include "spark/render/scene/VulkanSceneDescriptors.hpp"
#include "spark/render/scene/VulkanClusteredForwardLights.hpp"
#include "spark/render/scene/VulkanCustomMeshPool.hpp"
#include "spark/render/scene/VulkanDeferredUploadBatch.hpp"
#include "spark/render/present/VulkanDepthResources.hpp"
#include "spark/render/shadow/VulkanDirectionalShadowFrameState.hpp"
#include "spark/render/shadow/VulkanDirectionalShadowPass.hpp"
#include "spark/render/shadow/VulkanPunctualShadowFrameState.hpp"
#include "spark/render/shadow/VulkanPunctualShadowPass.hpp"
#include "spark/render/post/VulkanHdrTonemapPass.hpp"
#include "spark/render/post/VulkanScreenSpaceEffectsPass.hpp"
#include "spark/render/sprites2d/VulkanParticlePass.hpp"
#include "spark/render/present/VulkanPresentRenderPass.hpp"
#include "spark/render/present/VulkanPresentationFramebuffers.hpp"
#include "spark/render/lighting/SceneLightingProfile.hpp"
#include "spark/render/scene/VulkanSceneMeshGpu.hpp"
#include "spark/render/scene/VulkanSceneOpaquePass.hpp"
#include "spark/render/scene/VulkanScenePipeline.hpp"
#include "spark/render/scene/VulkanSceneTextureUploader.hpp"
#include "spark/render/scene/VulkanSceneUniformWriter.hpp"
#include "spark/render/ui/VulkanScreenUiPass.hpp"
#include "spark/render/gpu/VulkanSpvShaderLoader.hpp"
#include "spark/render/sprites2d/Vulkan2DCompositePass.hpp"
#include "spark/render/sprites2d/VulkanSpritePass.hpp"
#include "spark/render/sprites2d/VulkanTilemapPass.hpp"
#include "spark/render/platform/Window.hpp"

#include <cstddef>
#include <cstdint>

#include <chrono>

namespace Spark {

/**
 * Vulkan frame renderer: coordinates scene passes, presentation, and per-frame GPU uploads.
 * Instance, device, queues, and swapchain images live in <c>VulkanDeviceContext</c>.
 * Low-level Vulkan helpers live under <c>VulkanRendererGpu</c> (e.g. <c>VulkanGpuBufferImage</c>).
 * Presentation and sync are split into <c>VulkanDeviceContext</c>, <c>VulkanPresentationSwapchain</c>,
 * <c>VulkanPresentRenderPass</c>, <c>VulkanDepthResources</c>, <c>VulkanPresentationFramebuffers</c>,
 * <c>VulkanSceneDescriptors</c>, <c>VulkanFrameSync</c>, and <c>VulkanFrameCapture</c> so this class
 * orchestrates frame flow and scene data.
 * Rendering subsystems are composed (not inherited): `VulkanSpvShaderLoader`, `VulkanSceneUniformWriter`,
 * `VulkanScenePipeline`, `VulkanDeferredUploadBatch`, `VulkanDirectionalShadowPass`, `VulkanPunctualShadowPass`,
 * `VulkanHdrTonemapPass`, `VulkanScreenSpaceEffectsPass` (SSAO), `VulkanParticlePass`,
 * `VulkanTilemapPass`, `VulkanSpritePass`, and `VulkanScreenUiPass` own their respective render slices (`VulkanScreenUiClip`
 * supplies shared scissor helpers).
 */
class VulkanRenderer : public IFramePresenter {
public:
    explicit VulkanRenderer(Window& appWindow);
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;
    ~VulkanRenderer();

    void PresentFrame() override;
    void NotifySwapchainResize() override;
    void SetSceneRenderParams(const SceneRenderParams& params) override;
    void RequestScreenshotSave(const char* pathUtf8) override;
    [[nodiscard]] bool BeginVideoRecording(const VideoRecordingSettings& settings) override;
    void EndVideoRecording() override;
    [[nodiscard]] bool IsVideoRecording() const override;
    [[nodiscard]] VideoRecorder* GetActiveVideoRecorder() override;
    [[nodiscard]] bool TryGetDrawableSize(int& outWidth, int& outHeight) const override;

    void DrawFrame();

private:
    void CreatePersistentSceneResources();
    void DestroyPersistentSceneResources();

    void RecreateSwapchain();
    void CleanupSwapchain();
    void CreateDepthResources();
    void DestroyDepthResources();
    void CreateRenderPass();
    void RecreateHdrFlightTargets();
    void CreateFramebuffers();
    void CreateCommandPool();
    void RecordSceneCommandBuffer(VkCommandBuffer commandBuffer, std::uint32_t imageIndex, std::uint32_t frameIndex);

    void CreateSceneGeometry();

    void WriteUniformBuffer(std::uint32_t frameIndex);

    void RecordShadowMapPass(VkCommandBuffer commandBuffer, std::uint32_t frameIndex);

    VulkanDeviceContext deviceContext;
    VulkanFrameSync frameSync;
    VulkanSceneDescriptors sceneDescriptors;
    VulkanSpvShaderLoader shaderLoader;
    VulkanSceneUniformWriter sceneUniformWriter;
    VulkanClusteredForwardLights clusteredForwardLights;
    VulkanFrameCapture frameCapture;
    VulkanDirectionalShadowPass directionalShadow;
    VulkanDirectionalShadowFrameState shadowFrameState{};
    VulkanPunctualShadowPass punctualShadow;
    VulkanPunctualShadowFrameState punctualShadowFrameState{};
    VulkanHdrTonemapPass hdrTonemapPass;
    VulkanScreenSpaceEffectsPass screenSpaceEffectsPass;
    VulkanSceneOpaquePass sceneOpaquePass;
    VulkanSceneTextureUploader sceneTextureUploader;
    VulkanCustomMeshPool customMeshPool;
    VulkanDeferredUploadBatch deferredUploadBatch;
    VulkanScreenUiPass screenUi;
    VulkanParticlePass particlePass;
    VulkanTilemapPass tilemapPass;
    VulkanSpritePass spritePass;
    Vulkan2DCompositePass composite2DPass;

    VulkanPresentationSwapchain& presentSwapchain() noexcept { return deviceContext.GetSwapchain(); }
    [[nodiscard]] const VulkanPresentationSwapchain& presentSwapchain() const noexcept {
        return deviceContext.GetSwapchain();
    }
    [[nodiscard]] VkDevice device() const noexcept { return deviceContext.GetDevice(); }
    [[nodiscard]] VkPhysicalDevice physicalDevice() const noexcept { return deviceContext.GetPhysicalDevice(); }
    [[nodiscard]] VkQueue graphicsQueue() const noexcept { return deviceContext.GetGraphicsQueue(); }
    [[nodiscard]] VkQueue presentQueue() const noexcept { return deviceContext.GetPresentQueue(); }

    /** Tonemap + screen UI (swapchain color only). */
    VulkanPresentRenderPass presentRenderPass;
    VulkanScenePipeline scenePipeline;
    VulkanPresentationFramebuffers presentationFramebuffers;

    VkFormat sceneDepthFormat = VK_FORMAT_UNDEFINED;
    /** One depth buffer per in-flight frame (avoids GPU races with parallel swapchain images). */
    Array<VulkanDepthResources> sceneDepthResources;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    std::uint32_t cubeIndexCount = 0;
    std::uint32_t planeIndexCount = 0;
    std::uint32_t planeFirstIndex = 0;
    std::uint32_t spriteQuadFirstIndex = 0;
    std::uint32_t spriteQuadIndexCount = 0;
    std::int32_t cubeVertexOffset = 0;
    std::int32_t planeVertexOffset = 0;

    Array<CustomMeshGpuSlice> customDrawPacked;
    Array<CustomMeshGpuSlice> customDrawPackedTransparent;
    std::uint64_t submittedFrameCounter = 0;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    Array<VkCommandBuffer> commandBuffers;

    bool framebufferResized = false;
    SceneRenderParams pendingScene{};
    bool sceneParamsValid = false;
    /** Cached after <c>SetSceneRenderParams</c> from profile + overrides. */
    ResolvedSceneLighting resolvedLighting{};

    std::chrono::steady_clock::time_point recordingWallStart{};
    bool recordingWallClockValid = false;
};

}  // namespace Spark
