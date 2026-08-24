#pragma once

#include "spark/core/Array.hpp"

#include <cstdint>

namespace Spark {

/** One mip level of CPU-side texture data (RGBA8 or block-compressed bytes). */
class TextureMipLevel {
public:
    TextureMipLevel() = default;
    TextureMipLevel(std::uint32_t levelWidth, std::uint32_t levelHeight, Array<std::uint8_t> levelBytes);

    [[nodiscard]] std::uint32_t GetWidth() const noexcept { return width; }
    [[nodiscard]] std::uint32_t GetHeight() const noexcept { return height; }
    [[nodiscard]] const Array<std::uint8_t>& GetBytes() const noexcept { return bytes; }
    [[nodiscard]] Array<std::uint8_t>& GetBytes() noexcept { return bytes; }

private:
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    Array<std::uint8_t> bytes;
};

}  // namespace Spark
