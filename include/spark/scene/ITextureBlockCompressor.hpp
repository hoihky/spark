#pragma once

#include "spark/core/Array.hpp"
#include "spark/scene/TextureFormat.hpp"

#include <cstdint>

namespace Spark {

/** Strategy interface for encoding one RGBA8 mip into a block-compressed payload. */
class ITextureBlockCompressor {
public:
    virtual ~ITextureBlockCompressor() = default;

    [[nodiscard]] virtual TexturePixelFormat GetFormat() const noexcept = 0;
    [[nodiscard]] virtual bool CompressMip(
            const std::uint8_t* rgba,
            std::uint32_t width,
            std::uint32_t height,
            Array<std::uint8_t>& outBlocks) = 0;
};

}  // namespace Spark
