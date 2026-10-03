#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <algorithm>
#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/** Local capsule axis before the owning transform is applied. */
enum class CapsuleDirection3D : std::uint8_t {
    X = 0,
    Y = 1,
    Z = 2,
};

/**
 * Capsule in local space: a cylinder with hemispherical caps along <c>direction</c>.
 * <c>height</c> is the total extent including both caps (Unity-style). When
 * <c>height &lt; 2 * radius</c>, the shape collapses to a sphere of radius <c>height / 2</c>.
 */
class CapsuleCollider3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::CapsuleCollider3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit CapsuleCollider3DComponent(
            float radiusIn = 0.5F,
            float heightIn = 2.0F,
            CapsuleDirection3D directionIn = CapsuleDirection3D::Y,
            Vector3 localOffset = Vector3::Zero) noexcept
            : radius(radiusIn), height(heightIn), direction(directionIn), offset(localOffset) {}

    SPARK_SCRIPT_BIND(get_radius)
    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    SPARK_SCRIPT_BIND(set_radius)
    void SetRadius(const float r) noexcept { radius = std::max(0.01F, r); }

    SPARK_SCRIPT_BIND(get_height)
    [[nodiscard]] float GetHeight() const noexcept { return height; }
    SPARK_SCRIPT_BIND(set_height)
    void SetHeight(const float h) noexcept { height = std::max(0.01F, h); }

    SPARK_SCRIPT_BIND(get_direction)
    [[nodiscard]] CapsuleDirection3D GetDirection() const noexcept { return direction; }
    SPARK_SCRIPT_BIND(set_direction)
    void SetDirection(const CapsuleDirection3D axis) noexcept { direction = axis; }

    SPARK_SCRIPT_BIND(get_offset)
    [[nodiscard]] const Vector3& GetOffset() const noexcept { return offset; }
    SPARK_SCRIPT_BIND(set_offset)
    void SetOffset(const Vector3& o) noexcept { offset = o; }

private:
    float radius = 0.5F;
    float height = 2.0F;
    CapsuleDirection3D direction = CapsuleDirection3D::Y;
    Vector3 offset{Vector3::Zero};
};

}  // namespace Spark
