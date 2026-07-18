#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

namespace Spark {

/** Sphere in local space (center = offset, radius in local units before non-uniform scale). */
class SphereCollider3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::SphereCollider3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    explicit SphereCollider3DComponent(float radius = 0.5F, Vector3 localOffset = Vector3::Zero) noexcept
            : radius(radius), offset(localOffset) {}

    [[nodiscard]] float GetRadius() const noexcept { return radius; }
    [[nodiscard]] const Vector3& GetOffset() const noexcept { return offset; }

    void SetRadius(float r) noexcept { radius = r; }
    void SetOffset(const Vector3& o) noexcept { offset = o; }

private:
    float radius = 0.5F;
    Vector3 offset{Vector3::Zero};
};

}  // namespace Spark
