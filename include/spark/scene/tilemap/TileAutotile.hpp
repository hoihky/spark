#pragma once

#include <cstdint>

namespace Spark {

/** One resolved autotile variant for a 4-neighbor connectivity mask (N=1, E=2, S=4, W=8). */
struct TileAutotileVariant {
    std::uint16_t tileId = 0;
    std::uint8_t transformFlags = 0;
};

/** Maps 16 connectivity masks to display tiles for one terrain group. */
struct TileAutotileRuleSet {
    static constexpr std::uint32_t kMaskCount = 16U;

    std::uint8_t groupId = 0;
    TileAutotileVariant variants[kMaskCount]{};

    void SetVariant(const std::uint8_t mask, const std::uint16_t tileId, const std::uint8_t transformFlags = 0) noexcept {
        if (mask < kMaskCount) {
            variants[mask].tileId = tileId;
            variants[mask].transformFlags = transformFlags;
        }
    }
};

}  // namespace Spark
