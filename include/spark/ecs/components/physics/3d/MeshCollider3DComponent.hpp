#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

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

    [[nodiscard]] const Vector3& GetLocalOffset() const noexcept { return offset; }
    void SetLocalOffset(const Vector3& o) noexcept { offset = o; }

    [[nodiscard]] bool IsTrigger() const noexcept { return isTrigger; }
    void SetIsTrigger(const bool t) noexcept { isTrigger = t; }

private:
    Vector3 offset{Vector3::Zero};
    bool isTrigger = false;
};

}  // namespace Spark
