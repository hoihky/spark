#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

#include <cstdint>

namespace Spark {

/**
 * Static mesh collider: world AABB baked from <c>MeshComponent</c> vertices each broad-phase rebuild.
 * Use for level geometry imported from glTF/OBJ when a simple box is insufficient.
 */
class MeshCollider3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::MeshCollider3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit MeshCollider3DComponent(Vector3 localOffset = Vector3::Zero) noexcept : offset(localOffset) {}

    SPARK_SCRIPT_BIND(get_local_offset)
    [[nodiscard]] const Vector3& GetLocalOffset() const noexcept { return offset; }
    SPARK_SCRIPT_BIND(set_local_offset)
    void SetLocalOffset(const Vector3& o) noexcept { offset = o; }

    SPARK_SCRIPT_BIND(get_is_trigger)
    [[nodiscard]] bool GetIsTrigger() const noexcept { return isTrigger; }
    SPARK_SCRIPT_BIND(set_is_trigger)
    void SetIsTrigger(const bool t) noexcept { isTrigger = t; }

    SPARK_SCRIPT_BIND(get_category_bits)
    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }
    SPARK_SCRIPT_BIND(set_category_bits)
    void SetCategoryBits(const std::uint16_t bits) noexcept { categoryBits = bits; }

    SPARK_SCRIPT_BIND(get_mask_bits)
    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }
    SPARK_SCRIPT_BIND(set_mask_bits)
    void SetMaskBits(const std::uint16_t bits) noexcept { maskBits = bits; }

private:
    Vector3 offset{Vector3::Zero};
    bool isTrigger = false;
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
};

}  // namespace Spark
