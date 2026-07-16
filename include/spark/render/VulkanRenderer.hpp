#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/IFramePresenter.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/media/VideoRecordingSettings.hpp"

#include <vulkan/vulkan.h>

#include "spark/render/VulkanClusteredForwardLights.hpp"
#include "spark/render/VulkanCustomMeshPool.hpp"
#include "spark/render/VulkanDeferredUploadBatch.hpp"
#include "spark/render/VulkanScreenshotCapture.hpp"
#include "spark/render/VulkanVideoCapture.hpp"
#include "spark/render/VulkanDepthResources.hpp"
#include "spark/render/VulkanDirectionalShadowFrameState.hpp"
#include "spark/render/VulkanDirectionalShadowPass.hpp"
#include "spark/render/VulkanPunctualShadowFrameState.hpp"
#include "spark/render/VulkanPunctualShadowPass.hpp"
#include "spark/render/VulkanHdrTonemapPass.hpp"
#include "spark/render/VulkanScreenSpaceEffectsPass.hpp"
#include "spark/render/VulkanParticlePass.hpp"
#include "spark/render/VulkanPresentRenderPass.hpp"
#include "spark/render/VulkanPresentationFramebuffers.hpp"
#include "spark/render/SceneLightingProfile.hpp"
#include "spark/render/VulkanPresentationSwapchain.hpp"
#include "spark/render/VulkanSceneMeshGpu.hpp"
#include "spark/render/VulkanSceneOpaquePass.hpp"
#include "spark/render/VulkanScenePipeline.hpp"
#include "spark/render/VulkanSceneTextureUploader.hpp"
#include "spark/render/VulkanSceneUniformWriter.hpp"
#include "spark/render/VulkanScreenUiPass.hpp"
#include "spark/render/VulkanSpvShaderLoader.hpp"
#include "spark/render/Vulkan2DCompositePass.hpp"
#include "spark/render/VulkanSpritePass.hpp"
#include "spark/render/VulkanTilemapPass.hpp"
#include "spark/render/Window.hpp"

#include <cstddef>
#include <cstdint>

#include <chrono>

namespace Spark {

/**
 * Vulkan swapchain renderer: instance, device, swapchain, render pass, graphics pipeline, and frame sync.
 * Swapchain images are presented each frame; shaders and draw commands define what is rendered.
 * Low-level Vulkan helpers live under `VulkanRendererGpu` (service classes such as `VulkanGpuBufferImage`).
 * Swapchain presentation pieces are split into `VulkanPresentationSwapchain`, `VulkanPresentRenderPass`,
 * `VulkanDepthResources`, and `VulkanPresentationFramebuffers` so this class coordinates frame flow and scene data.
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
    void CreateSyncObjects();
    void CreateRenderFinishedSemaphores();
    void DestroyRenderFinishedSemaphores();

    void CreateDescriptorSetLayout();
    void CreateUniformBuffers();
    void CreateSkinSsboBuffers();
    void CreateDescriptorPoolAndSets();
    void CreateSceneGeometry();

    void WriteUniformBuffer(std::uint32_t frameIndex);

    void RecordShadowMapPass(VkCommandBuffer commandBuffer, std::uint32_t frameIndex);
    void WaitAllFramesComplete() const;

    Window& appWindow;

    VulkanSpvShaderLoader shaderLoader;
    VulkanSceneUniformWriter sceneUniformWriter;
    VulkanClusteredForwardLights clusteredForwardLights;
    VulkanScreenshotCapture screenshotCapture;
    VulkanVideoCapture videoCapture;
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

    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VulkanPresentationSwapchain presentSwapchain;
    /** Tonemap + screen UI (swapchain color only). */
    VulkanPresentRenderPass presentRenderPass;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
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

    static constexpr std::uint32_t kMaxSkinJoints = 64;
    /** std430 mat4 skinPalette[kMaxSkinJoints] — 64 joints × 64 bytes. */
    static constexpr VkDeviceSize kSkinSsboBytes = 4096;
    Array<VkBuffer> skinSsboBuffers;
    Array<VkDeviceMemory> skinSsboMemory;
    Array<void*> skinSsboMapped;

    Array<VkBuffer> uniformBuffers;
    Array<VkDeviceMemory> uniformBuffersMemory;
    Array<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    Array<VkDescriptorSet> descriptorSets;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    Array<VkCommandBuffer> commandBuffers;

    Array<VkSemaphore> imageAvailableSemaphores;
    /** One per swapchain image — indexed by acquired <c>imageIndex</c> for submit/present signal. */
    Array<VkSemaphore> renderFinishedSemaphores;
    Array<VkFence> inFlightFences;
    Array<VkFence> imagesInFlight;

    std::uint32_t currentFrame = 0;
    static constexpr std::uint32_t maxFramesInFlight = 2;
    bool framebufferResized = false;
    SceneRenderParams pendingScene{};
    bool sceneParamsValid = false;
    /** Cached after <c>SetSceneRenderParams</c> from profile + overrides. */
    ResolvedSceneLighting resolvedLighting{};

    std::chrono::steady_clock::time_point recordingWallStart{};
    bool recordingWallClockValid = false;
};

}  // namespace Spark
