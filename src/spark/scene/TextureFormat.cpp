#include "spark/scene/TextureFormat.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

std::uint32_t TextureFormat::CountMipLevels(const std::uint32_t width, const std::uint32_t height) noexcept {
    const std::uint32_t dim = std::max(width, height);
    if (dim == 0) {
        return 0;
    }
    return static_cast<std::uint32_t>(std::floor(std::log2(static_cast<float>(dim)))) + 1U;
}

std::uint32_t TextureFormat::MipDimension(const std::uint32_t base, const std::uint32_t level) noexcept {
    const std::uint32_t shifted = base >> level;
    return std::max(shifted, 1U);
}

std::uint32_t TextureFormat::BlockWidth(const TexturePixelFormat format) noexcept {
    switch (format) {
        case TexturePixelFormat::Bc7Unorm:
        case TexturePixelFormat::Astc4x4Unorm:
            return 4U;
        case TexturePixelFormat::Rgba8Unorm:
        default:
            return 1U;
    }
}

std::uint32_t TextureFormat::BlockHeight(const TexturePixelFormat format) noexcept {
    return BlockWidth(format);
}

std::uint32_t TextureFormat::BlockByteSize(const TexturePixelFormat format) noexcept {
    switch (format) {
        case TexturePixelFormat::Bc7Unorm:
        case TexturePixelFormat::Astc4x4Unorm:
            return 16U;
        case TexturePixelFormat::Rgba8Unorm:
        default:
            return 4U;
    }
}

std::uint32_t TextureFormat::RgbaMipByteSize(const std::uint32_t width, const std::uint32_t height) noexcept {
    return width * height * 4U;
}

std::uint32_t TextureFormat::CompressedMipByteSize(
        const TexturePixelFormat format,
        const std::uint32_t width,
        const std::uint32_t height) noexcept {
    if (format == TexturePixelFormat::Rgba8Unorm || width == 0 || height == 0) {
        return RgbaMipByteSize(width, height);
    }
    const std::uint32_t blockW = BlockWidth(format);
    const std::uint32_t blockH = BlockHeight(format);
    const std::uint32_t blocksX = (width + blockW - 1U) / blockW;
    const std::uint32_t blocksY = (height + blockH - 1U) / blockH;
    return blocksX * blocksY * BlockByteSize(format);
}

std::uint32_t TextureFormat::CompressedMipChainByteSize(
        const TexturePixelFormat format,
        const std::uint32_t baseWidth,
        const std::uint32_t baseHeight) noexcept {
    const std::uint32_t mipCount = CountMipLevels(baseWidth, baseHeight);
    std::uint32_t total = 0;
    for (std::uint32_t level = 0; level < mipCount; ++level) {
        total += CompressedMipByteSize(format, MipDimension(baseWidth, level), MipDimension(baseHeight, level));
    }
    return total;
}

}  // namespace Spark
