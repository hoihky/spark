#pragma once

#include <cstdint>

namespace Spark {

/** Per-cell orientation (flip + 90° steps). Packed into <c>TileCell::transformFlags</c>. */
enum class TileTransformFlags : std::uint8_t {
    None = 0,
    FlipH = 1U << 0,
    FlipV = 1U << 1,
    /** Two low bits after flips: rotation count 0–3, each step is 90° CCW in cell space. */
    Rotate90Mask = 0x0CU,
};

[[nodiscard]] constexpr TileTransformFlags operator|(const TileTransformFlags a, const TileTransformFlags b) noexcept {
    return static_cast<TileTransformFlags>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

[[nodiscard]] constexpr std::uint8_t TileTransformRotation90Count(const std::uint8_t packed) noexcept {
    return (packed & static_cast<std::uint8_t>(TileTransformFlags::Rotate90Mask)) >> 2U;
}

[[nodiscard]] constexpr std::uint8_t PackTileTransform(
        const bool flipH,
        const bool flipV,
        const std::uint8_t rotation90Count) noexcept {
    std::uint8_t flags = 0;
    if (flipH) {
        flags |= static_cast<std::uint8_t>(TileTransformFlags::FlipH);
    }
    if (flipV) {
        flags |= static_cast<std::uint8_t>(TileTransformFlags::FlipV);
    }
    flags |= static_cast<std::uint8_t>((rotation90Count & 3U) << 2U);
    return flags;
}

/**
 * Tiled numbers atlas tiles from the top row; Spark's grid UVs use the bottom row as index 0
 * (see <c>SpriteAnimatorComponent::ComputeUniformGridUv</c> / Kenney pack helpers).
 */
[[nodiscard]] inline std::uint32_t TiledLocalTileIndexToSparkAtlasIndex(
        const std::uint32_t tiledLocal,
        const std::uint32_t columns,
        const std::uint32_t tileCount) noexcept {
    if (columns == 0U) {
        return tiledLocal;
    }
    const std::uint32_t rows =
            tileCount > 0U ? (tileCount + columns - 1U) / columns : 1U;
    const std::uint32_t col = tiledLocal % columns;
    const std::uint32_t rowTop = tiledLocal / columns;
    if (rowTop >= rows) {
        return tiledLocal;
    }
    const std::uint32_t rowFromBottom = (rows - 1U) - rowTop;
    return col + rowFromBottom * columns;
}

}  // namespace Spark
