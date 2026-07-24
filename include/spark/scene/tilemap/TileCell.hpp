#pragma once

#include "spark/scene/tilemap/TileTransform.hpp"

#include <cstdint>

namespace Spark {

/**
 * One map cell: atlas tile id plus optional transform and tint.
 * <c>tileId == kEmptyTileId</c> means an empty hole (no render, no collision).
 */
struct TileCell {
    static constexpr std::uint16_t kEmptyTileId = 0xFFFFU;

    std::uint16_t tileId = kEmptyTileId;
    /** When not <c>kEmptyTileId</c>, autotile uses this as the painted terrain id (display <c>tileId</c> may differ). */
    std::uint16_t paintTileId = kEmptyTileId;
    std::uint8_t transformFlags = 0;
    std::uint8_t tintR = 255;
    std::uint8_t tintG = 255;
    std::uint8_t tintB = 255;
    std::uint8_t tintA = 255;

    [[nodiscard]] static TileCell Empty() noexcept { return {}; }

    [[nodiscard]] static TileCell FromTileId(const std::uint16_t id) noexcept {
        TileCell cell{};
        cell.tileId = id;
        return cell;
    }

    [[nodiscard]] bool IsEmpty() const noexcept { return tileId == kEmptyTileId; }

    /** True when the cell should be submitted to the tilemap render pass. */
    [[nodiscard]] bool HasVisual() const noexcept { return !IsEmpty(); }

    /** Terrain / source id for autotile and animation lookup (falls back to <c>tileId</c>). */
    [[nodiscard]] std::uint16_t GetPaintTileId() const noexcept {
        return paintTileId != kEmptyTileId ? paintTileId : tileId;
    }

    void SetFlip(const bool flipH, const bool flipV) noexcept {
        transformFlags = static_cast<std::uint8_t>(
                (transformFlags & static_cast<std::uint8_t>(TileTransformFlags::Rotate90Mask)) |
                (flipH ? static_cast<std::uint8_t>(TileTransformFlags::FlipH) : 0U) |
                (flipV ? static_cast<std::uint8_t>(TileTransformFlags::FlipV) : 0U));
    }

    void SetRotation90Count(const std::uint8_t count) noexcept {
        transformFlags = static_cast<std::uint8_t>(
                (transformFlags & ~static_cast<std::uint8_t>(TileTransformFlags::Rotate90Mask)) |
                static_cast<std::uint8_t>((count & 3U) << 2U));
    }
};

}  // namespace Spark
