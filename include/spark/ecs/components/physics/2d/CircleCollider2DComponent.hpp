#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector2.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Circle in local XY (Z ignored), centered at offset with given radius in local units before transform scale.
 * Pairs with Rigidbody2D (dynamic) or used alone for static obstacles; do not mix with BoxCollider2D on the same
 * body for <c>PhysicsWorld2D</c> simulation — if both exist, the circle takes precedence for simulation.
 */
class CircleCollider2DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::CircleCollider2D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit CircleCollider2DComponent(float localRadius = 0.5F, Vector2 localOffset = Vector2::Zero) noexcept
            : radius(localRadius), offset(localOffset) {}

    SPARK_SCRIPT_BIND(get_radius)
    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    SPARK_SCRIPT_BIND(get_offset)
    [[nodiscard]] const Vector2& GetOffset() const noexcept { return offset; }

    SPARK_SCRIPT_BIND(set_radius)
    void SetRadius(float r) noexcept { radius = r; }
    SPARK_SCRIPT_BIND(set_offset)
    void SetOffset(const Vector2& o) noexcept { offset = o; }

    SPARK_SCRIPT_BIND(get_category_bits)
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }
    SPARK_SCRIPT_BIND(set_category_bits)
    void SetCategoryBits(std::uint16_t bits) noexcept { categoryBits = bits; }

    SPARK_SCRIPT_BIND(get_mask_bits)
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }
    SPARK_SCRIPT_BIND(set_mask_bits)
    void SetMaskBits(std::uint16_t bits) noexcept { maskBits = bits; }

    SPARK_SCRIPT_BIND(get_is_trigger)
    [[nodiscard]] bool GetIsTrigger() const noexcept { return isTrigger; }
    SPARK_SCRIPT_BIND(set_is_trigger)
    void SetIsTrigger(bool isTrigger) noexcept { this->isTrigger = isTrigger; }

private:
    float radius = 0.5F;
    Vector2 offset{Vector2::Zero};
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    bool isTrigger = false;
};

}  // namespace Spark
