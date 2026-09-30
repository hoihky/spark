#include "spark/gameplay/fog/FogOfWarMaskTextureBuilder.hpp"

#include "spark/scene/texture/Texture2D.hpp"

namespace Spark {

namespace {

[[nodiscard]] std::uint8_t FogAlphaForState(const FogOfWarCellState state) noexcept {
    switch (state) {
        case FogOfWarCellState::Visible:
            return 0U;
        case FogOfWarCellState::Explored:
            return 200U;
        case FogOfWarCellState::Hidden:
        default:
            return 255U;
    }
}

}  // namespace

void FogOfWarMaskTextureBuilder::RebuildMaskTexture(
        const FogOfWarExplorationMap& exploration,
        Texture2D& texture) const noexcept {
    const std::int32_t w = exploration.GetWidth();
    const std::int32_t h = exploration.GetHeight();
    if (w <= 0 || h <= 0) {
        return;
    }
    Array<std::uint8_t> pixels{};
    pixels.Resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U);
    for (std::int32_t y = 0; y < h; ++y) {
        for (std::int32_t x = 0; x < w; ++x) {
            const std::size_t i = static_cast<std::size_t>((y * w + x) * 4);
            const std::uint8_t alpha = FogAlphaForState(exploration.GetCellState(x, y));
            pixels[i + 0U] = 2U;
            pixels[i + 1U] = 3U;
            pixels[i + 2U] = 8U;
            pixels[i + 3U] = alpha;
        }
    }
    texture.SetPixels(static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h), MoveTemp(pixels));
    texture.SetSceneUploadNearest(true);
}

}  // namespace Spark
