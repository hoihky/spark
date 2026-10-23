#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/physics/core/ColliderFilter.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

#include <cstdint>

namespace Spark {

/**
 * Default collision layer for static 3D colliders on this object when shape components do not set their own bits.
 */
class PhysicsLayer3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::PhysicsLayer3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    SPARK_SCRIPT_BIND(get_category_bits)
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return filter.categoryBits; }
    SPARK_SCRIPT_BIND(set_category_bits)
    void SetCategoryBits(const std::uint16_t bits) noexcept { filter.categoryBits = bits; }

    SPARK_SCRIPT_BIND(get_mask_bits)
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return filter.maskBits; }
    SPARK_SCRIPT_BIND(set_mask_bits)
    void SetMaskBits(const std::uint16_t bits) noexcept { filter.maskBits = bits; }

    SPARK_SCRIPT_BIND(get_is_trigger)
    [[nodiscard]] bool GetIsTrigger() const noexcept { return filter.isTrigger; }
    SPARK_SCRIPT_BIND(set_is_trigger)
    void SetIsTrigger(const bool value) noexcept { filter.isTrigger = value; }

    SPARK_SCRIPT_BIND(get_filter)
    [[nodiscard]] const ColliderFilter& GetFilter() const noexcept { return filter; }

private:
    ColliderFilter filter{};
};

}  // namespace Spark
