#include "spark/gui/GuiSpriteSlice.hpp"

#include <algorithm>

namespace Spark::Gui {

GuiSpriteSlice GuiSpriteSlice::FromPixels(
        const SharedPtr<Texture2D>& texture,
        const std::uint32_t x,
        const std::uint32_t y,
        const std::uint32_t width,
        const std::uint32_t height,
        GuiNineSliceInsets nineSlice) noexcept {
    GuiSpriteSlice slice{};
    slice.texture = texture;
    slice.nineSlice = nineSlice;
    if (!texture || width == 0U || height == 0U) {
        return slice;
    }
    const float tw = static_cast<float>(std::max(1U, texture->GetWidth()));
    const float th = static_cast<float>(std::max(1U, texture->GetHeight()));
    const float u0 = static_cast<float>(x) / tw;
    const float v0 = static_cast<float>(y) / th;
    const float u1 = static_cast<float>(x + width) / tw;
    const float v1 = static_cast<float>(y + height) / th;
    slice.uvRect = {u0, v0, u1, v1};
    return slice;
}

}  // namespace Spark::Gui
