#pragma once

#include <cstdint>

namespace Spark {

/**
 * Bitmask collision filtering for 2D physics (ARPG-style layers).
 *
 * Convention:
 * - Each logical "layer" i uses bit (1u << i), i in [0, 15].
 * - categoryBits: which layers this collider **is** (often a single bit).
 * - maskBits: which layers this collider **collides with** (often several bits set).
 *
 * Two bodies A and B interact when masks overlap categories mutually:
 * (maskA & categoryB) != 0 && (maskB & categoryA) != 0.
 */
namespace CollisionFilter2D {

[[nodiscard]] constexpr std::uint16_t LayerBit(std::uint32_t layerIndex) noexcept {
    return static_cast<std::uint16_t>(1u << (layerIndex & 15u));
}

[[nodiscard]] constexpr std::uint16_t AllLayersMask() noexcept { return 0xFFFFu; }

[[nodiscard]] constexpr std::uint16_t DefaultCategory() noexcept { return 1u; }

[[nodiscard]] inline bool ShouldCollide(
        std::uint16_t categoryA,
        std::uint16_t maskA,
        std::uint16_t categoryB,
        std::uint16_t maskB) noexcept {
    return ((maskA & categoryB) != 0) && ((maskB & categoryA) != 0);
}

}  // namespace CollisionFilter2D

}  // namespace Spark
