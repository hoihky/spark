#include "spark/scene/render/RenderTexture.hpp"

#include <algorithm>

namespace Spark {

namespace {

constexpr std::uint32_t kMaxRenderTextureExtent = 8192;

}  // namespace

RenderTexture::RenderTexture(RenderTextureDesc descIn) : desc(descIn) {
    desc.width = (std::max)(desc.width, 1U);
    desc.height = (std::max)(desc.height, 1U);
}

bool RenderTexture::IsValidExtent(const std::uint32_t width, const std::uint32_t height) noexcept {
    return width >= 1U && height >= 1U && width <= kMaxRenderTextureExtent && height <= kMaxRenderTextureExtent;
}

void RenderTexture::BumpGpuAllocationToken() noexcept {
    ++gpuAllocationToken;
    if (gpuAllocationToken == 0) {
        gpuAllocationToken = 1;
    }
}

void RenderTexture::Resize(const std::uint32_t width, const std::uint32_t height) noexcept {
    const std::uint32_t clampedW = (std::max)(width, 1U);
    const std::uint32_t clampedH = (std::max)(height, 1U);
    if (desc.width == clampedW && desc.height == clampedH) {
        return;
    }
    desc.width = clampedW;
    desc.height = clampedH;
    BumpGpuAllocationToken();
}

void RenderTexture::SetDesc(const RenderTextureDesc value) noexcept {
    RenderTextureDesc next = value;
    next.width = (std::max)(next.width, 1U);
    next.height = (std::max)(next.height, 1U);
    if (desc.width == next.width && desc.height == next.height && desc.colorFormat == next.colorFormat &&
        desc.includeDepth == next.includeDepth && desc.name == next.name) {
        return;
    }
    desc = next;
    BumpGpuAllocationToken();
}

}  // namespace Spark
