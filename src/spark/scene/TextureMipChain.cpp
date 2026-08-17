#include "spark/scene/TextureMipChain.hpp"

#include "spark/core/Utility.hpp"

#include <algorithm>

namespace Spark {

void TextureMipChain::DownsampleRgbBoxFilter(
        const std::uint8_t* src,
        const std::uint32_t srcW,
        const std::uint32_t srcH,
        Array<std::uint8_t>& dst,
        const std::uint32_t dstW,
        const std::uint32_t dstH) {
    if (src == nullptr || srcW == 0 || srcH == 0 || dstW == 0 || dstH == 0) {
        dst.Clear();
        return;
    }
    dst.Clear();
    dst.Resize(static_cast<std::size_t>(dstW) * static_cast<std::size_t>(dstH) * 4U);
    for (std::uint32_t y = 0; y < dstH; ++y) {
        const std::uint32_t y0 = (y * srcH) / dstH;
        const std::uint32_t y1 = std::min(((y + 1U) * srcH + dstH - 1U) / dstH, srcH);
        for (std::uint32_t x = 0; x < dstW; ++x) {
            const std::uint32_t x0 = (x * srcW) / dstW;
            const std::uint32_t x1 = std::min(((x + 1U) * srcW + dstW - 1U) / dstW, srcW);
            float accum[4]{0.0F, 0.0F, 0.0F, 0.0F};
            std::uint32_t count = 0;
            for (std::uint32_t sy = y0; sy < y1; ++sy) {
                for (std::uint32_t sx = x0; sx < x1; ++sx) {
                    const std::size_t si =
                            (static_cast<std::size_t>(sy) * static_cast<std::size_t>(srcW) +
                             static_cast<std::size_t>(sx)) *
                            4U;
                    for (int channel = 0; channel < 4; ++channel) {
                        accum[channel] += static_cast<float>(src[si + static_cast<std::size_t>(channel)]);
                    }
                    ++count;
                }
            }
            const float inv = count > 0 ? 1.0F / static_cast<float>(count) : 1.0F;
            const std::size_t di =
                    (static_cast<std::size_t>(y) * static_cast<std::size_t>(dstW) + static_cast<std::size_t>(x)) *
                    4U;
            for (int channel = 0; channel < 4; ++channel) {
                dst[di + static_cast<std::size_t>(channel)] =
                        static_cast<std::uint8_t>(std::clamp(accum[channel] * inv, 0.0F, 255.0F));
            }
        }
    }
}

void TextureMipChain::BuildFromRgba(
        const Array<std::uint8_t>& baseRgba,
        const std::uint32_t baseWidth,
        const std::uint32_t baseHeight) {
    levels.Clear();
    if (baseWidth == 0 || baseHeight == 0 || baseRgba.IsEmpty()) {
        return;
    }
    const std::uint32_t mipCount = TextureFormat::CountMipLevels(baseWidth, baseHeight);
    levels.Reserve(mipCount);
    levels.PushBack(TextureMipLevel(baseWidth, baseHeight, Array<std::uint8_t>(baseRgba)));

    for (std::uint32_t level = 1; level < mipCount; ++level) {
        const TextureMipLevel& prev = levels[level - 1U];
        const std::uint32_t nextW = TextureFormat::MipDimension(baseWidth, level);
        const std::uint32_t nextH = TextureFormat::MipDimension(baseHeight, level);
        Array<std::uint8_t> nextBytes;
        DownsampleRgbBoxFilter(
                prev.GetBytes().GetData(),
                prev.GetWidth(),
                prev.GetHeight(),
                nextBytes,
                nextW,
                nextH);
        levels.PushBack(TextureMipLevel(nextW, nextH, MoveTemp(nextBytes)));
    }
}

}  // namespace Spark
