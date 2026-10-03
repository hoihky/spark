#pragma once

#include "spark/ecs/GameComponent.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

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
    SPARK_SCRIPT_BIND(get_category_bits)
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }
    SPARK_SCRIPT_BIND(set_category_bits)
    void SetCategoryBits(const std::uint16_t bits) noexcept { categoryBits = bits; }

    /** Layers this collider interacts with (bitmask). Default = all 16 layers. */
    SPARK_SCRIPT_BIND(get_mask_bits)
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }
    SPARK_SCRIPT_BIND(set_mask_bits)
    void SetMaskBits(const std::uint16_t bits) noexcept { maskBits = bits; }

    /** If true, overlaps fire <c>SignalId::Physics2DTriggerOverlap</c> without blocking movement. */
    SPARK_SCRIPT_BIND(get_is_trigger)
    [[nodiscard]] bool GetIsTrigger() const noexcept { return isTrigger; }
    SPARK_SCRIPT_BIND(set_is_trigger)
    void SetIsTrigger(const bool value) noexcept { isTrigger = value; }

private:
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    bool isTrigger = false;
};

}  // namespace Spark
