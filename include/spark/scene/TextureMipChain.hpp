#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/scene/TextureFormat.hpp"
#include "spark/scene/TextureMipLevel.hpp"

#include <cstdint>

namespace Spark {

/** CPU-side RGBA8 mip chain builder and storage. */
class TextureMipChain {
public:
    [[nodiscard]] const Array<TextureMipLevel>& GetLevels() const noexcept { return levels; }
    [[nodiscard]] bool IsEmpty() const noexcept { return levels.IsEmpty(); }
    [[nodiscard]] std::size_t GetLevelCount() const noexcept { return levels.GetSize(); }

    void Clear() noexcept { levels.Clear(); }

    void AssignLevels(Array<TextureMipLevel> mips) { levels = MoveTemp(mips); }

    /** Build a full box-filtered mip chain from a base RGBA8 image. */
    void BuildFromRgba(const Array<std::uint8_t>& baseRgba, std::uint32_t baseWidth, std::uint32_t baseHeight);

    /** Box-filter downsample of an RGBA8 image into <c>dstW x dstH</c>. */
    static void DownsampleRgbBoxFilter(
            const std::uint8_t* src,
            std::uint32_t srcW,
            std::uint32_t srcH,
            Array<std::uint8_t>& dst,
            std::uint32_t dstW,
            std::uint32_t dstH);

private:
    Array<TextureMipLevel> levels;
};

// Backward-compatible free-function alias.
inline void BuildRgbMipChain(
        const Array<std::uint8_t>& baseRgba,
        const std::uint32_t baseWidth,
        const std::uint32_t baseHeight,
        Array<TextureMipLevel>& mipsOut) {
    TextureMipChain chain;
    chain.BuildFromRgba(baseRgba, baseWidth, baseHeight);
    mipsOut = chain.GetLevels();
}

inline void DownsampleRgbBoxFilter(
        const std::uint8_t* src,
        const std::uint32_t srcW,
        const std::uint32_t srcH,
        Array<std::uint8_t>& dst,
        const std::uint32_t dstW,
        const std::uint32_t dstH) {
    TextureMipChain::DownsampleRgbBoxFilter(src, srcW, srcH, dst, dstW, dstH);
}

}  // namespace Spark
