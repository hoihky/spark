#pragma once

#include "spark/core/Array.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/gpu/VulkanSpvShaderLoader.hpp"
#include "spark/scene/Texture2D.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Spark {

class Font;

/** Host-owned Vulkan handles required for font atlas upload. */
struct VulkanScreenUiHostContext {
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
};

/** Screen-space solid rects and text overlays recorded into the present (tonemap) pass. */
class VulkanScreenUiPass {
public:
    static constexpr VkDeviceSize kTextVertexBytes = 512U * 1024U;
    static constexpr VkDeviceSize kTextIndexBytes = 256U * 1024U;
    static constexpr VkDeviceSize kSolidVertexBytes = 512U * 1024U;
    static constexpr VkDeviceSize kSolidIndexBytes = 256U * 1024U;

    void CreateResources(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            std::uint32_t framesInFlight,
            const VulkanSpvShaderLoader& shaders);
    void DestroyResources(VkDevice device);

    void CreatePipelines(VkDevice device, VkRenderPass presentRenderPass);
    void DestroyPipelines(VkDevice device);

    [[nodiscard]] bool NeedsFontUpload(const SceneRenderParams& scene) const noexcept;
    void PrepareFontUpload(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            const SceneRenderParams& scene,
            std::uint64_t frameCounter,
            std::uint32_t maxFramesInFlight);
    void RecordFontUpload(VkCommandBuffer commandBuffer, VkDevice device);
    void ReleaseRetiredFontAtlases(VkDevice device, std::uint64_t frameCounter);

    [[nodiscard]] bool NeedsUiTextureUpload(const SceneRenderParams& scene) const noexcept;
    void PrepareUiTextureUpload(
            VkPhysicalDevice physicalDevice,
            VkDevice device,
            const SceneRenderParams& scene,
            std::uint64_t frameCounter,
            std::uint32_t maxFramesInFlight);
    void RecordUiTextureUpload(VkCommandBuffer commandBuffer, VkDevice device);
    void ReleaseRetiredUiTextureAtlases(VkDevice device, std::uint64_t frameCounter);
    void UpdateUiSpriteDescriptorImages(VkDevice device);

    void Record(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const SceneRenderParams& scene,
            bool sceneParamsValid);

private:
    struct TextPushConstants {
        float screenSize[4]{};
    };

    struct UiMeshFlightBuffers {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
        void* vertexMapped = nullptr;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMemory = VK_NULL_HANDLE;
        void* indexMapped = nullptr;
    };

    struct FontAtlasGpu {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        std::uint32_t width = 0;
        std::uint32_t height = 0;
    };

    struct RetiredFontAtlas {
        FontAtlasGpu atlas{};
        std::uint64_t safeAfterFrame = 0;
    };

    void DestroyFontAtlas(FontAtlasGpu& atlas, VkDevice device);
    void QueueRetireFontAtlas(FontAtlasGpu&& atlas, std::uint64_t safeAfterFrame);
    void EnsureFontStagingCapacity(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bytes);
    void UpdateTextDescriptorImage(VkDevice device);
    void RecordSolidRectsFor(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const Array<ScreenRectDraw>& rects);
    void RecordSolidRectsForPipeline(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const Array<ScreenRectDraw>& rects);
    void RecordSpritesFor(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const SceneRenderParams& scene,
            const Array<ScreenSpriteDraw>& sprites);
    void RecordTextOverlaysFor(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const SceneRenderParams& scene,
            const Array<ScreenTextDraw>& texts);
    void FlushAccumulatedUiSolids(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const ScreenRectDraw& clipRef,
            VkPipeline pipeline);
    void FlushAccumulatedUiTexts(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const ScreenTextDraw& clipRef,
            VkPipeline pipeline);
    void FlushAccumulatedUiSprites(
            VkCommandBuffer commandBuffer,
            std::uint32_t frameIndex,
            VkExtent2D extent,
            const ScreenSpriteDraw& clipRef,
            VkPipeline pipeline);
    void AppendUiSolidRectGeometry(const ScreenRectDraw& rd);
    void AppendUiSpriteGeometry(const ScreenSpriteDraw& sd);
    void AppendUiTextDrawGeometry(
            const ScreenTextDraw& td,
            const Font* regFont,
            const Font* boldFont,
            bool boldAtlasOk);

    [[nodiscard]] VkPipeline PipelineForSolidBlendMode(SceneBlendMode mode) const noexcept;
    [[nodiscard]] VkPipeline PipelineForSpriteBlendMode(SceneBlendMode mode) const noexcept;
    [[nodiscard]] VkPipeline PipelineForTextBlendMode(SceneBlendMode mode) const noexcept;

    VkShaderModule textVertModule = VK_NULL_HANDLE;
    VkShaderModule textFragModule = VK_NULL_HANDLE;
    VkShaderModule uiSpriteFragModule = VK_NULL_HANDLE;
    VkPipeline textPipelines[kSceneBlendModeCount]{};
    VkPipeline spritePipelines[kSceneBlendModeCount]{};
    VkPipelineLayout textPipelineLayout = VK_NULL_HANDLE;
    VkPipelineLayout spritePipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout spriteDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool textDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorPool spriteDescriptorPool = VK_NULL_HANDLE;
    Array<VkDescriptorSet> textDescriptorSets;
    Array<VkDescriptorSet> spriteDescriptorSets;
    VkSampler fontSampler = VK_NULL_HANDLE;
    FontAtlasGpu activeFontAtlas{};
    VkBuffer fontStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory fontStagingMemory = VK_NULL_HANDLE;
    void* fontStagingMapped = nullptr;
    VkDeviceSize fontStagingCapacity = 0;
    FontAtlasGpu pendingFontAtlas{};
    VkDeviceSize pendingFontLayerBytes = 0;
    bool fontUploadPending = false;
    bool fontUploadClearsAtlas = false;
    const Font* pendingUiFont = nullptr;
    const Font* pendingUiBoldFont = nullptr;
    std::uint64_t fontRetireAfterFrame = 0;
    Array<RetiredFontAtlas> retiredFontAtlases{};
    const Font* uploadedUiFont = nullptr;
    const Font* uploadedUiBoldFont = nullptr;

    FontAtlasGpu activeUiSpriteAtlas{};
    FontAtlasGpu pendingUiSpriteAtlas{};
    VkBuffer uiSpriteStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uiSpriteStagingMemory = VK_NULL_HANDLE;
    void* uiSpriteStagingMapped = nullptr;
    VkDeviceSize uiSpriteStagingCapacity = 0;
    VkDeviceSize pendingUiSpriteLayerBytes = 0;
    std::uint32_t pendingUiSpriteLayerCount = 0;
    bool uiSpriteUploadPending = false;
    bool uiSpriteUploadClearsAtlas = false;
    std::uint64_t uiSpriteRetireAfterFrame = 0;
    Array<RetiredFontAtlas> retiredUiSpriteAtlases{};
    Array<const Texture2D*> uploadedUiTexturePointers{};
    Array<const Texture2D*> pendingUiTexturePointers{};
    VkDevice device = VK_NULL_HANDLE;

    Array<UiMeshFlightBuffers> textFlightBuffers;
    Array<UiMeshFlightBuffers> spriteFlightBuffers;
    Array<UiMeshFlightBuffers> solidFlightBuffers;
    Array<float> textScratchVertices;
    Array<std::uint32_t> textScratchIndices;
    Array<float> spriteScratchVertices;
    Array<std::uint32_t> spriteScratchIndices;

    VkShaderModule solidVertModule = VK_NULL_HANDLE;
    VkShaderModule solidFragModule = VK_NULL_HANDLE;
    VkPipeline solidPipelines[kSceneBlendModeCount]{};
    VkPipelineLayout solidPipelineLayout = VK_NULL_HANDLE;
    Array<float> solidScratchVertices;
    Array<std::uint32_t> solidScratchIndices;
};

}  // namespace Spark
