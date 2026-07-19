#pragma once

#include "spark/math/Vector4.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/Texture2D.hpp"

namespace Spark::Gui {

/** Pixel insets from the sprite edges used for nine-slice scaling (zero = stretch the whole quad). */
struct GuiNineSliceInsets {
    float left = 0.0F;
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;

    [[nodiscard]] bool IsEmpty() const noexcept {
        return left <= 0.0F && top <= 0.0F && right <= 0.0F && bottom <= 0.0F;
    }
};

/**
 * Reference to a rectangular region inside a texture atlas. UVs are normalized [0,1].
 * Any UI asset pack can be expressed as a table of these slices.
 */
struct GuiSpriteSlice {
    SharedPtr<Texture2D> texture{};
    /** (minU, minV, maxU, maxV) */
    Vector4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};
    GuiNineSliceInsets nineSlice{};

    [[nodiscard]] bool IsValid() const noexcept { return static_cast<bool>(texture); }

    /** Builds a slice from pixel coordinates in @p texture (origin top-left). */
    [[nodiscard]] static GuiSpriteSlice FromPixels(
            const SharedPtr<Texture2D>& texture,
            std::uint32_t x,
            std::uint32_t y,
            std::uint32_t width,
            std::uint32_t height,
            GuiNineSliceInsets nineSlice = {}) noexcept;
};

}  // namespace Spark::Gui
