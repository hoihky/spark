#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <algorithm>
#include <cstdint>

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

    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    void SetRadius(const float r) noexcept { radius = std::max(0.01F, r); }

    [[nodiscard]] float GetHeight() const noexcept { return height; }
    void SetHeight(const float h) noexcept { height = std::max(0.01F, h); }

    [[nodiscard]] CapsuleDirection3D GetDirection() const noexcept { return direction; }
    void SetDirection(const CapsuleDirection3D axis) noexcept { direction = axis; }

    [[nodiscard]] const Vector3& GetOffset() const noexcept { return offset; }
    void SetOffset(const Vector3& o) noexcept { offset = o; }

    [[nodiscard]] std::uint16_t GetCategoryBits() const noexcept { return categoryBits; }
    void SetCategoryBits(const std::uint16_t bits) noexcept { categoryBits = bits; }

    [[nodiscard]] std::uint16_t GetMaskBits() const noexcept { return maskBits; }
    void SetMaskBits(const std::uint16_t bits) noexcept { maskBits = bits; }

    [[nodiscard]] bool GetIsTrigger() const noexcept { return isTrigger; }
    void SetIsTrigger(const bool value) noexcept { isTrigger = value; }

private:
    float radius = 0.5F;
    float height = 2.0F;
    CapsuleDirection3D direction = CapsuleDirection3D::Y;
    Vector3 offset{Vector3::Zero};
    std::uint16_t categoryBits = 1u;
    std::uint16_t maskBits = 0xFFFFu;
    bool isTrigger = false;
};

}  // namespace Spark
