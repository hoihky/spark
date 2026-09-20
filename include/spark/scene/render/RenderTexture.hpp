#pragma once

#include "spark/core/Utf8String.hpp"
#include "spark/scene/render/RenderTextureDesc.hpp"

#include <cstdint>

namespace Spark {

/**
 * CPU-side offscreen render target descriptor (analogous to <c>Texture2D</c> for sampled assets).
 * GPU images are allocated lazily by <c>IRenderTargetService</c> when a view is created.
 */
class RenderTexture {
public:
    RenderTexture() = default;
    explicit RenderTexture(RenderTextureDesc desc);

    [[nodiscard]] const RenderTextureDesc& GetDesc() const noexcept { return desc; }
    [[nodiscard]] Utf8String& GetName() noexcept { return desc.name; }
    [[nodiscard]] const Utf8String& GetName() const noexcept { return desc.name; }
    [[nodiscard]] std::uint32_t GetWidth() const noexcept { return desc.width; }
    [[nodiscard]] std::uint32_t GetHeight() const noexcept { return desc.height; }
    [[nodiscard]] RenderTextureFormat GetColorFormat() const noexcept { return desc.colorFormat; }
    [[nodiscard]] bool IncludesDepth() const noexcept { return desc.includeDepth; }

    /** Marks GPU resources stale; call <c>IRenderTargetService::EnsureGpuResources</c> before use. */
    void Resize(std::uint32_t width, std::uint32_t height) noexcept;
    void SetDesc(RenderTextureDesc value) noexcept;

    /**
     * Monotonic token bumped when desc/size changes. The Vulkan backend recreates images when this
     * differs from the last allocated token.
     */
    [[nodiscard]] std::uint64_t GetGpuAllocationToken() const noexcept { return gpuAllocationToken; }

    [[nodiscard]] static bool IsValidExtent(std::uint32_t width, std::uint32_t height) noexcept;

private:
    void BumpGpuAllocationToken() noexcept;

    RenderTextureDesc desc{};
    std::uint64_t gpuAllocationToken = 1;
};

}  // namespace Spark
