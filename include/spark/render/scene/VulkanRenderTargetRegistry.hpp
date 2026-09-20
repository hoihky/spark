#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/HashMap.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/memory/UniquePtr.hpp"
#include "spark/render/IRenderTarget.hpp"
#include "spark/render/IRenderTargetService.hpp"
#include "spark/render/scene/VulkanOffscreenRenderTarget.hpp"
#include "spark/scene/render/RenderTexture.hpp"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>

namespace Spark {

class RenderTexture;

/** Vulkan implementation of <c>IRenderTargetService</c>; owned by <c>VulkanRenderer</c>. */
class VulkanRenderTargetRegistry final : public IRenderTargetService {
public:
    void BindDevice(
            VkPhysicalDevice physicalDeviceIn,
            VkDevice deviceIn,
            VkRenderPass hdrRenderPassIn,
            VkFormat depthFormatIn) noexcept;
    void DestroyAll(VkDevice device) noexcept;

    void EnsureGpuResources(RenderTexture& texture) override;
    [[nodiscard]] SharedPtr<IRenderTarget> CreateRenderTarget(SharedPtr<RenderTexture> texture) override;
    void ReleaseRenderTarget(const RenderTexture& texture) noexcept override;

    [[nodiscard]] VulkanOffscreenRenderTarget* TryGetGpu(const RenderTexture& texture) noexcept;
    [[nodiscard]] const VulkanOffscreenRenderTarget* TryGetGpu(const RenderTexture& texture) const noexcept;

private:
    struct Entry {
        SharedPtr<RenderTexture> texture;
        UniquePtr<VulkanOffscreenRenderTarget> gpu;
        std::uint32_t viewRefCount = 0;
    };

    struct RenderTexturePointerHasher {
        [[nodiscard]] std::size_t operator()(const RenderTexture* ptr) const noexcept {
            return reinterpret_cast<std::size_t>(ptr);
        }
    };

    class VulkanRenderTargetView final : public IRenderTarget {
    public:
        VulkanRenderTargetView(SharedPtr<RenderTexture> textureIn, VulkanRenderTargetRegistry& ownerIn) noexcept;
        ~VulkanRenderTargetView() override;

        [[nodiscard]] const RenderTexture& GetColorTexture() const noexcept override { return *texture; }
        [[nodiscard]] std::uint32_t GetWidth() const noexcept override { return texture->GetWidth(); }
        [[nodiscard]] std::uint32_t GetHeight() const noexcept override { return texture->GetHeight(); }
        [[nodiscard]] bool HasDepth() const noexcept override { return texture->IncludesDepth(); }

    private:
        SharedPtr<RenderTexture> texture;
        VulkanRenderTargetRegistry& owner;
    };

    friend class VulkanRenderTargetView;

    void DropView(const RenderTexture& texture) noexcept;
    void AllocateGpuForEntry(Entry& entry);
    [[nodiscard]] Entry* FindEntry(const RenderTexture& texture) noexcept;
    [[nodiscard]] const Entry* FindEntry(const RenderTexture& texture) const noexcept;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass hdrRenderPass = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    Array<Entry> entries{};
    HashMap<const RenderTexture*, std::size_t, RenderTexturePointerHasher> indexByTexture{};
};

}  // namespace Spark
