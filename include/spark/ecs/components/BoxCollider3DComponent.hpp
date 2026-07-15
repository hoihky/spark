#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/**
 * Axis-aligned box in local space (before the owning transform). Default half extents match a unit cube mesh
 * scaled uniformly by 0.5× cell size (local ±1 in mesh space).
 */
class BoxCollider3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::BoxCollider3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit BoxCollider3DComponent(Vector3 localHalfExtents = {0.5F, 0.5F, 0.5F}, Vector3 localOffset = Vector3::Zero) noexcept
            : halfExtents(localHalfExtents), offset(localOffset) {}

    [[nodiscard]] const Vector3& GetHalfExtents() const noexcept { return halfExtents; }
    [[nodiscard]] const Vector3& GetOffset() const noexcept { return offset; }

    void SetHalfExtents(const Vector3& h) noexcept { halfExtents = h; }
    void SetOffset(const Vector3& o) noexcept { offset = o; }

private:
    Vector3 halfExtents{0.5F, 0.5F, 0.5F};
    Vector3 offset{Vector3::Zero};
};

}  // namespace Spark
