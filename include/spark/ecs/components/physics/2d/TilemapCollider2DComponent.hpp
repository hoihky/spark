#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>

namespace Spark {

/**
 * Bakes static colliders from non-empty tiles on the sibling <c>TilemapComponent</c> (per-tile shapes from
 * <c>TileDefinition</c>, optional per-map-layer via <c>TilemapLayer::contributeCollision</c>) each physics step.
 * Grid: origin corner (0,0), +X/+Y along axes, cell size = <c>GetTileWorldSize()</c>.
 */
class TilemapCollider2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::TilemapCollider2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    /** Layer bitmask (usually one bit). Default = layer 0. */
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }
    void SetCategoryBits(const std::uint16_t bits) noexcept { categoryBits = bits; }

    /** Layers this collider interacts with (bitmask). Default = all 16 layers. */
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }
    void SetMaskBits(const std::uint16_t bits) noexcept { maskBits = bits; }

    /** If true, overlaps fire <c>SignalId::Physics2DTriggerOverlap</c> without blocking movement. */
    [[nodiscard]] bool GetIsTrigger() const noexcept { return isTrigger; }
    void SetIsTrigger(const bool value) noexcept { isTrigger = value; }

private:
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    bool isTrigger = false;
};

}  // namespace Spark
