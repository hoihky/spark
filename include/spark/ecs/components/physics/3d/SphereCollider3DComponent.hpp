#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

#include <cstdint>

namespace Spark {

/** Sphere in local space (center = offset, radius in local units before non-uniform scale). */
class SphereCollider3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SphereCollider3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SphereCollider3DComponent(float radius = 0.5F, Vector3 localOffset = Vector3::Zero) noexcept
            : radius(radius), offset(localOffset) {}

    SPARK_SCRIPT_BIND(get_radius)
    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    SPARK_SCRIPT_BIND(get_offset)
    [[nodiscard]] const Vector3& GetOffset() const noexcept { return offset; }

    SPARK_SCRIPT_BIND(set_radius)
    void SetRadius(float r) noexcept { radius = r; }
    SPARK_SCRIPT_BIND(set_offset)
    void SetOffset(const Vector3& o) noexcept { offset = o; }

    SPARK_SCRIPT_BIND(get_category_bits)
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }
    SPARK_SCRIPT_BIND(set_category_bits)
    void SetCategoryBits(const std::uint16_t bits) noexcept { categoryBits = bits; }

    SPARK_SCRIPT_BIND(get_mask_bits)
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }
    SPARK_SCRIPT_BIND(set_mask_bits)
    void SetMaskBits(const std::uint16_t bits) noexcept { maskBits = bits; }

    SPARK_SCRIPT_BIND(get_is_trigger)
    [[nodiscard]] bool GetIsTrigger() const noexcept { return isTrigger; }
    SPARK_SCRIPT_BIND(set_is_trigger)
    void SetIsTrigger(const bool value) noexcept { isTrigger = value; }

private:
    float radius = 0.5F;
    Vector3 offset{Vector3::Zero};
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    bool isTrigger = false;
};

}  // namespace Spark
