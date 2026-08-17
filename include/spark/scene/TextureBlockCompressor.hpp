#pragma once

#include "spark/scene/ITextureBlockCompressor.hpp"
#include "spark/scene/TextureMipChain.hpp"

namespace Spark {

/**
 * Facade over block-compression strategies (BC7, ASTC).
 * Selects the registered compressor for a <c>TexturePixelFormat</c>.
 */
class TextureBlockCompressor {
public:
    [[nodiscard]] static TextureBlockCompressor& Get() noexcept;

    [[nodiscard]] bool CompressMip(
            TexturePixelFormat format,
            const std::uint8_t* rgba,
            std::uint32_t width,
            std::uint32_t height,
            Array<std::uint8_t>& outBlocks);

    [[nodiscard]] bool CompressChain(
            TexturePixelFormat format,
            const TextureMipChain& rgbaChain,
            TextureMipChain& outChain);

    [[nodiscard]] bool CompressChain(
            TexturePixelFormat format,
            const Array<TextureMipLevel>& rgbaMips,
            Array<TextureMipLevel>& outMips);
};

// Backward-compatible free-function aliases.
[[nodiscard]] inline bool TryCompressRgbMip(
        const TexturePixelFormat format,
        const std::uint8_t* rgba,
        const std::uint32_t width,
        const std::uint32_t height,
        Array<std::uint8_t>& outBlocks) {
    return TextureBlockCompressor::Get().CompressMip(format, rgba, width, height, outBlocks);
}

[[nodiscard]] inline bool TryCompressRgbMipChain(
        const TexturePixelFormat format,
        const Array<TextureMipLevel>& rgbaMips,
        Array<TextureMipLevel>& outMips) {
    return TextureBlockCompressor::Get().CompressChain(format, rgbaMips, outMips);
}

}  // namespace Spark
