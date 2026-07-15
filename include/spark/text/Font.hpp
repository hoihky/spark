#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scene/Texture2D.hpp"

#include <cstdint>

namespace Spark {

/**
 * TrueType font rasterized into a single-channel atlas (packed via stb_truetype).
 * Screen layout uses pixel coordinates with Y increasing downward (GLFW / stb convention).
 */
class Font {
public:
    Font() = default;

    /** Rasterize ASCII 32–127 into an atlas; emPixels controls baked glyph size (display size scales from this). */
    [[nodiscard]] bool TryLoadTrueTypeFromFile(const char* path, float emPixels);

    [[nodiscard]] bool IsValid() const noexcept { return atlas.GetWidth() > 0 && !packedStorage.IsEmpty(); }
    [[nodiscard]] const Texture2D& GetAtlas() const noexcept { return atlas; }
    /** Pixel height used when packing the atlas; used to scale arbitrary on-screen sizes. */
    [[nodiscard]] float GetPackedEmPixels() const noexcept { return emPixels; }

    /**
     * Append textured quads for UTF-8 text (ASCII / Latin-1 single-byte code units; others skipped).
     * (x, y) is the top-left of the first line in screen pixels (Y downward, same as GLFW).
     * Vertex layout: px, py, u, v, r, g, b, a (8 floats per vertex).
     */
    /**
     * @param atlasLayer 0 = primary atlas layer (regular); 1 = bold layer in combined GPU atlas.
     */
    void AppendTextGeometry(
            const Utf8String& text,
            float x,
            float y,
            float sizePixels,
            const Vector3& color,
            float alpha,
            float atlasLayer,
            Array<float>& interleaved,
            Array<std::uint32_t>& indices) const;

    /** Horizontal advance in screen pixels (ASCII / Latin-1 + approximate for other UTF-8). */
    [[nodiscard]] float MeasureUtf8Width(const Utf8String& text, float sizePixels) const noexcept;

    /** Recommended line step for wrapped UI text at the given on-screen size. */
    [[nodiscard]] float GetLineSpacingPixels(float sizePixels) const noexcept;

    /**
     * Alpha-composite UTF-8 text onto an RGBA8 buffer (screen pixels, Y downward).
     * Skips rendering when the font is invalid.
     */
    void BlendUtf8TextOntoRgba(
            std::uint8_t* rgba,
            std::uint32_t imageWidth,
            std::uint32_t imageHeight,
            const char* textUtf8,
            float x,
            float y,
            float sizePixels,
            std::uint8_t r,
            std::uint8_t g,
            std::uint8_t b,
            std::uint8_t alpha) const;

private:
    Texture2D atlas{};
    float emPixels = 0.0F;
    /** Ascent in pixels at emPixels bake size (baseline → typographic top); used to interpret y as top-left. */
    float lineAscentPx = 0.0F;
    int atlasW = 0;
    int atlasH = 0;
    Array<std::uint8_t> packedStorage{};
};

}  // namespace Spark
