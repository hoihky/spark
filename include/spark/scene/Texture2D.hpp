#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/math/Vector3.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * CPU-side RGBA8 texture. Upload to GPU is renderer-specific; dimensions need not match GPU tile size
 * (renderer may resample to a fixed atlas layer size).
 */
class Texture2D {
public:
    Texture2D() = default;
    explicit Texture2D(Utf8String textureName);

    [[nodiscard]] Utf8String& GetName() noexcept { return name; }
    [[nodiscard]] const Utf8String& GetName() const noexcept { return name; }

    [[nodiscard]] std::uint32_t GetWidth() const noexcept { return width; }
    [[nodiscard]] std::uint32_t GetHeight() const noexcept { return height; }
    [[nodiscard]] const Array<std::uint8_t>& GetRgba() const noexcept { return rgba; }

    void SetPixels(std::uint32_t w, std::uint32_t h, Array<std::uint8_t> bytes);

    /** Nearest-neighbor resize into outRgba (w*h*4 bytes). */
    void ResampleNearest(std::uint32_t targetW, std::uint32_t targetH, Array<std::uint8_t>& outRgba) const;
    /** Bilinear resize into outRgba (w*h*4 bytes). */
    void ResampleBilinear(std::uint32_t targetW, std::uint32_t targetH, Array<std::uint8_t>& outRgba) const;

    /** Cached FNV-1a fingerprint; refreshed when pixels change via <c>SetPixels</c>. */
    [[nodiscard]] std::uint64_t GetContentFingerprint() const noexcept { return contentFingerprint; }

    static Texture2D CreateCheckerboard(
            std::uint32_t size,
            std::uint32_t tilePixels,
            Vector3 colorA,
            Vector3 colorB);

    static Texture2D CreateSolid(std::uint32_t w, std::uint32_t h, Vector3 rgb, float alpha = 1.0F);

    /**
     * Load PNG/JPEG/etc. via stb_image into RGBA8. Returns false if the file is missing or invalid.
     * @param flipVerticalOnLoad When true, matches legacy stb top-left origin; use false for 2D/tile UVs that expect row 0 at top.
     */
    [[nodiscard]] static bool TryLoadFromFile(const char* path, Texture2D& out, bool flipVerticalOnLoad = true);

    /** Decode embedded image bytes (e.g. from glTF buffer views). */
    [[nodiscard]] static bool TryLoadFromMemory(
            const std::uint8_t* bytes, std::size_t byteCount, Texture2D& out, const char* debugName = "Memory");

    /** Simple procedural brick wall (fallback if no image file). */
    static Texture2D CreateBrickPattern(std::uint32_t width, std::uint32_t height);

    /** Brown, patchy soil / dirt (fallback if no soil image). */
    static Texture2D CreateSoilPattern(std::uint32_t width, std::uint32_t height);

    /**
     * Large-scale blend of several brown soil types (loam, clay, sand, silt) plus fine grit — natural patchiness
     * for terrain albedo.
     */
    static Texture2D CreateVariedSoilGroundPattern(std::uint32_t width, std::uint32_t height);

private:
    Utf8String name;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    Array<std::uint8_t> rgba;
    std::uint64_t contentFingerprint = 0;

    void RefreshContentFingerprint() noexcept;
};

}  // namespace Spark
