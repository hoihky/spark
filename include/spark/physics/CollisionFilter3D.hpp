#pragma once

#include "spark/physics/CollisionFilter2D.hpp"

#include <cstdint>

namespace Spark {

/** 3D physics/query layers use the same 16-bit bitmask rules as 2D (<c>CollisionFilter2D</c>). */
namespace CollisionFilter3D {
[[nodiscard]] constexpr std::uint16_t LayerBit(const std::uint32_t layerIndex) noexcept {
    return CollisionFilter2D::LayerBit(layerIndex);
}
[[nodiscard]] constexpr std::uint16_t AllLayersMask() noexcept { return CollisionFilter2D::AllLayersMask(); }
[[nodiscard]] constexpr std::uint16_t DefaultCategory() noexcept { return CollisionFilter2D::DefaultCategory(); }
[[nodiscard]] constexpr bool ShouldCollide(
        std::uint16_t categoryA,
        std::uint16_t maskA,
        std::uint16_t categoryB,
        std::uint16_t maskB) noexcept {
    return CollisionFilter2D::ShouldCollide(categoryA, maskA, categoryB, maskB);
}
}  // namespace CollisionFilter3D

/** Named layer indices for action games (bit = <c>CollisionFilter3D::LayerBit(index)</c>). */
namespace PhysicsLayers3D {
constexpr std::uint32_t Environment = 0;
constexpr std::uint32_t Player = 1;
constexpr std::uint32_t Enemy = 2;
constexpr std::uint32_t Projectile = 3;
constexpr std::uint32_t Trigger = 4;
}  // namespace PhysicsLayers3D

}  // namespace Spark
