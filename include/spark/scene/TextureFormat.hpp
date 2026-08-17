#pragma once

#include "spark/scene/TextureMipLevel.hpp"

#include <cstdint>

namespace Spark {

/** CPU-side pixel layout for texture assets and upload staging. */
enum class TexturePixelFormat : std::uint8_t {
    Rgba8Unorm = 0,
    Bc7Unorm,
    Astc4x4Unorm,
};

/** GPU scene texture array encoding selected at device init. */
enum class SceneTextureArrayMode : std::uint8_t {
    Rgba8Mipped = 0,
    Bc7Mipped,
    Astc4x4Mipped,
};

/** Mip-level math and block-format sizing for texture upload pipelines. */
class TextureFormat {
public:
    [[nodiscard]] static std::uint32_t CountMipLevels(std::uint32_t width, std::uint32_t height) noexcept;
    [[nodiscard]] static std::uint32_t MipDimension(std::uint32_t base, std::uint32_t level) noexcept;

    [[nodiscard]] static std::uint32_t BlockWidth(TexturePixelFormat format) noexcept;
    [[nodiscard]] static std::uint32_t BlockHeight(TexturePixelFormat format) noexcept;
    [[nodiscard]] static std::uint32_t BlockByteSize(TexturePixelFormat format) noexcept;

    [[nodiscard]] static std::uint32_t RgbaMipByteSize(std::uint32_t width, std::uint32_t height) noexcept;
    [[nodiscard]] static std::uint32_t CompressedMipByteSize(
            TexturePixelFormat format,
            std::uint32_t width,
            std::uint32_t height) noexcept;
    [[nodiscard]] static std::uint32_t CompressedMipChainByteSize(
            TexturePixelFormat format,
            std::uint32_t baseWidth,
            std::uint32_t baseHeight) noexcept;
};

// Backward-compatible free-function aliases.
[[nodiscard]] inline std::uint32_t CountMipLevels(const std::uint32_t width, const std::uint32_t height) noexcept {
    return TextureFormat::CountMipLevels(width, height);
}
[[nodiscard]] inline std::uint32_t MipDimension(const std::uint32_t base, const std::uint32_t level) noexcept {
    return TextureFormat::MipDimension(base, level);
}
[[nodiscard]] inline std::uint32_t TextureBlockWidth(const TexturePixelFormat format) noexcept {
    return TextureFormat::BlockWidth(format);
}
[[nodiscard]] inline std::uint32_t TextureBlockHeight(const TexturePixelFormat format) noexcept {
    return TextureFormat::BlockHeight(format);
}
[[nodiscard]] inline std::uint32_t TextureBlockByteSize(const TexturePixelFormat format) noexcept {
    return TextureFormat::BlockByteSize(format);
}
[[nodiscard]] inline std::uint32_t RgbaMipByteSize(const std::uint32_t width, const std::uint32_t height) noexcept {
    return TextureFormat::RgbaMipByteSize(width, height);
}
[[nodiscard]] inline std::uint32_t CompressedMipByteSize(
        const TexturePixelFormat format,
        const std::uint32_t width,
        const std::uint32_t height) noexcept {
    return TextureFormat::CompressedMipByteSize(format, width, height);
}
[[nodiscard]] inline std::uint32_t CompressedMipChainByteSize(
        const TexturePixelFormat format,
        const std::uint32_t baseWidth,
        const std::uint32_t baseHeight) noexcept {
    return TextureFormat::CompressedMipChainByteSize(format, baseWidth, baseHeight);
}

}  // namespace Spark
