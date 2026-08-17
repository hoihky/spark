#include "spark/scene/TextureBlockCompressor.hpp"

#include "spark/core/Utility.hpp"

#include <algorithm>

#ifndef SPARK_HAS_BC7ENC
#define SPARK_HAS_BC7ENC 0
#endif
#ifndef SPARK_HAS_ASTCENC
#define SPARK_HAS_ASTCENC 0
#endif

#if SPARK_TEXTURE_COMPRESSION && SPARK_HAS_BC7ENC
#include "bc7enc.h"
#endif

#if SPARK_TEXTURE_COMPRESSION && SPARK_HAS_ASTCENC
#include <astcenc.h>
#endif

namespace Spark {

namespace {

#if SPARK_TEXTURE_COMPRESSION && SPARK_HAS_BC7ENC

bool gBc7Initialized = false;

void EnsureBc7Encoder() {
    if (!gBc7Initialized) {
        bc7enc_compress_block_init();
        gBc7Initialized = true;
    }
}

void PadRgbaToBlockGrid(
        const std::uint8_t* rgba,
        const std::uint32_t width,
        const std::uint32_t height,
        Array<std::uint8_t>& padded,
        std::uint32_t& paddedWidth,
        std::uint32_t& paddedHeight) {
    paddedWidth = ((width + 3U) / 4U) * 4U;
    paddedHeight = ((height + 3U) / 4U) * 4U;
    if (paddedWidth == width && paddedHeight == height) {
        padded.Clear();
        return;
    }
    padded.Clear();
    padded.Resize(static_cast<std::size_t>(paddedWidth) * static_cast<std::size_t>(paddedHeight) * 4U);
    for (std::uint32_t y = 0; y < paddedHeight; ++y) {
        const std::uint32_t srcY = std::min(y, height > 0 ? height - 1U : 0U);
        for (std::uint32_t x = 0; x < paddedWidth; ++x) {
            const std::uint32_t srcX = std::min(x, width > 0 ? width - 1U : 0U);
            const std::size_t srcIndex =
                    (static_cast<std::size_t>(srcY) * static_cast<std::size_t>(width) + static_cast<std::size_t>(srcX)) *
                    4U;
            const std::size_t dstIndex =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(paddedWidth) +
                     static_cast<std::size_t>(x)) *
                    4U;
            padded[dstIndex] = rgba[srcIndex];
            padded[dstIndex + 1U] = rgba[srcIndex + 1U];
            padded[dstIndex + 2U] = rgba[srcIndex + 2U];
            padded[dstIndex + 3U] = rgba[srcIndex + 3U];
        }
    }
}

bool CompressBc7(
        const std::uint8_t* rgba,
        const std::uint32_t width,
        const std::uint32_t height,
        Array<std::uint8_t>& outBlocks) {
    EnsureBc7Encoder();
    Array<std::uint8_t> padded;
    std::uint32_t paddedWidth = width;
    std::uint32_t paddedHeight = height;
    PadRgbaToBlockGrid(rgba, width, height, padded, paddedWidth, paddedHeight);
    const std::uint8_t* source = padded.IsEmpty() ? rgba : padded.GetData();

    const std::uint32_t blocksX = paddedWidth / 4U;
    const std::uint32_t blocksY = paddedHeight / 4U;
    const std::size_t blockBytes = static_cast<std::size_t>(blocksX) * static_cast<std::size_t>(blocksY) * 16U;
    outBlocks.Clear();
    outBlocks.Resize(blockBytes);

    bc7enc_compress_block_params params{};
    bc7enc_compress_block_params_init(&params);
    params.m_uber_level = 1;

    for (std::uint32_t by = 0; by < blocksY; ++by) {
        for (std::uint32_t bx = 0; bx < blocksX; ++bx) {
            std::uint8_t blockRgba[4U * 4U * 4U]{};
            for (std::uint32_t py = 0; py < 4U; ++py) {
                for (std::uint32_t px = 0; px < 4U; ++px) {
                    const std::uint32_t x = bx * 4U + px;
                    const std::uint32_t y = by * 4U + py;
                    const std::size_t srcIndex =
                            (static_cast<std::size_t>(y) * static_cast<std::size_t>(paddedWidth) +
                             static_cast<std::size_t>(x)) *
                            4U;
                    const std::size_t dstIndex = (static_cast<std::size_t>(py) * 4U + static_cast<std::size_t>(px)) * 4U;
                    blockRgba[dstIndex] = source[srcIndex];
                    blockRgba[dstIndex + 1U] = source[srcIndex + 1U];
                    blockRgba[dstIndex + 2U] = source[srcIndex + 2U];
                    blockRgba[dstIndex + 3U] = source[srcIndex + 3U];
                }
            }
            const std::size_t blockIndex =
                    (static_cast<std::size_t>(by) * static_cast<std::size_t>(blocksX) + static_cast<std::size_t>(bx)) *
                    16U;
            bc7enc_compress_block(outBlocks.GetData() + blockIndex, blockRgba, &params);
        }
    }
    return true;
}

#endif  // SPARK_TEXTURE_COMPRESSION && SPARK_HAS_BC7ENC

#if SPARK_TEXTURE_COMPRESSION && SPARK_HAS_ASTCENC

struct AstcEncoderState {
    astcenc_context* context = nullptr;
};

AstcEncoderState& GetAstcEncoderState() {
    static AstcEncoderState state;
    return state;
}

bool EnsureAstcEncoder() {
    AstcEncoderState& state = GetAstcEncoderState();
    if (state.context != nullptr) {
        return true;
    }
    astcenc_config config{};
    const astcenc_error status = astcenc_config_init(
            ASTCENC_PRF_LDR,
            4,
            4,
            ASTCENC_PRE_MEDIUM,
            ASTCENC_FLG_SELF_DECOMPRESS_ONLY,
            &config);
    if (status != ASTCENC_SUCCESS) {
        return false;
    }
    return astcenc_context_alloc(&config, 1, &state.context) == ASTCENC_SUCCESS;
}

bool CompressAstc4x4(
        const std::uint8_t* rgba,
        const std::uint32_t width,
        const std::uint32_t height,
        Array<std::uint8_t>& outBlocks) {
    if (!EnsureAstcEncoder()) {
        return false;
    }
#if SPARK_TEXTURE_COMPRESSION && SPARK_HAS_BC7ENC
    Array<std::uint8_t> padded;
    std::uint32_t paddedWidth = width;
    std::uint32_t paddedHeight = height;
    PadRgbaToBlockGrid(rgba, width, height, padded, paddedWidth, paddedHeight);
    const std::uint8_t* source = padded.IsEmpty() ? rgba : padded.GetData();
#else
    const std::uint8_t* source = rgba;
    const std::uint32_t paddedWidth = ((width + 3U) / 4U) * 4U;
    const std::uint32_t paddedHeight = ((height + 3U) / 4U) * 4U;
#endif
    const std::uint32_t blocksX = paddedWidth / 4U;
    const std::uint32_t blocksY = paddedHeight / 4U;
    const std::size_t blockBytes = static_cast<std::size_t>(blocksX) * static_cast<std::size_t>(blocksY) * 16U;
    outBlocks.Clear();
    outBlocks.Resize(blockBytes);
    const astcenc_error status = astcenc_compress_image(
            GetAstcEncoderState().context,
            source,
            paddedWidth,
            paddedHeight,
            static_cast<std::size_t>(paddedWidth) * 4U,
            outBlocks.GetData(),
            blockBytes,
            0);
    return status == ASTCENC_SUCCESS;
}

#endif  // SPARK_TEXTURE_COMPRESSION && SPARK_HAS_ASTCENC

    [[nodiscard]] static bool CompressMipImpl(
            TexturePixelFormat format,
            const std::uint8_t* rgba,
            std::uint32_t width,
            std::uint32_t height,
            Array<std::uint8_t>& outBlocks) {
        if (rgba == nullptr || width == 0 || height == 0) {
            return false;
        }
#if SPARK_TEXTURE_COMPRESSION
        switch (format) {
#if SPARK_HAS_BC7ENC
            case TexturePixelFormat::Bc7Unorm:
                return CompressBc7(rgba, width, height, outBlocks);
#endif
#if SPARK_HAS_ASTCENC
            case TexturePixelFormat::Astc4x4Unorm:
                return CompressAstc4x4(rgba, width, height, outBlocks);
#endif
            case TexturePixelFormat::Rgba8Unorm:
            default:
                break;
        }
#else
        (void)format;
        (void)rgba;
        (void)width;
        (void)height;
        (void)outBlocks;
#endif
        return false;
    }

}  // namespace

TextureBlockCompressor& TextureBlockCompressor::Get() noexcept {
    static TextureBlockCompressor instance;
    return instance;
}

bool TextureBlockCompressor::CompressMip(
        const TexturePixelFormat format,
        const std::uint8_t* rgba,
        const std::uint32_t width,
        const std::uint32_t height,
        Array<std::uint8_t>& outBlocks) {
    return CompressMipImpl(format, rgba, width, height, outBlocks);
}

bool TextureBlockCompressor::CompressChain(
        const TexturePixelFormat format,
        const TextureMipChain& rgbaChain,
        TextureMipChain& outChain) {
    Array<TextureMipLevel> outMips;
    if (!CompressChain(format, rgbaChain.GetLevels(), outMips)) {
        return false;
    }
    outChain.AssignLevels(MoveTemp(outMips));
    return true;
}

bool TextureBlockCompressor::CompressChain(
        const TexturePixelFormat format,
        const Array<TextureMipLevel>& rgbaMips,
        Array<TextureMipLevel>& outMips) {
    outMips.Clear();
    if (rgbaMips.IsEmpty()) {
        return false;
    }
    outMips.Reserve(rgbaMips.GetSize());
    for (std::size_t i = 0; i < rgbaMips.GetSize(); ++i) {
        const TextureMipLevel& src = rgbaMips[i];
        Array<std::uint8_t> blocks;
        if (!CompressMipImpl(
                    format,
                    src.GetBytes().GetData(),
                    src.GetWidth(),
                    src.GetHeight(),
                    blocks)) {
            outMips.Clear();
            return false;
        }
        outMips.PushBack(TextureMipLevel(src.GetWidth(), src.GetHeight(), MoveTemp(blocks)));
    }
    return true;
}

}  // namespace Spark
