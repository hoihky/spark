#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/core/ColliderFilter.hpp"
#include "spark/physics/core/ColliderMaterial.hpp"
#include "spark/physics/shapes/IShape3D.hpp"
#include "spark/physics/shapes/ShapeType3D.hpp"

namespace Spark {

class GameObject;

/** Object-oriented 3D collider snapshot (Phase 2). */
class Collider3D {
public:
    Collider3D() = default;
    Collider3D(Collider3D&&) noexcept = default;
    Collider3D& operator=(Collider3D&&) noexcept = default;
    Collider3D(const Collider3D&) = delete;
    Collider3D& operator=(const Collider3D&) = delete;

    static Collider3D Create(
            UniquePtr<IShape3D> shapeIn,
            ColliderFilter filterIn,
            ColliderMaterial materialIn,
            GameObject* ownerIn) noexcept;

    static Collider3D FromLegacySnapshot(const StaticCollider3DSim& snapshot);

    [[nodiscard]] StaticCollider3DSim ToLegacySnapshot() const;

    [[nodiscard]] bool IsValid() const noexcept { return shape != nullptr; }
    [[nodiscard]] const IShape3D& GetShape() const noexcept { return *shape; }
    [[nodiscard]] IShape3D& GetShape() noexcept { return *shape; }
    [[nodiscard]] ShapeType3D GetShapeType() const noexcept;
    [[nodiscard]] CollisionAabb3 GetBounds() const noexcept;

    [[nodiscard]] const ColliderFilter& GetFilter() const noexcept { return filter; }
    [[nodiscard]] const ColliderMaterial& GetMaterial() const noexcept { return material; }
    [[nodiscard]] ColliderMaterial& GetMaterial() noexcept { return material; }
    [[nodiscard]] GameObject* GetOwner() const noexcept { return owner; }

    [[nodiscard]] bool OverlapsAabb(const CollisionAabb3& aabb) const;
    [[nodiscard]] bool OverlapsSphere(const Vector3& center, float radius) const;

private:
    UniquePtr<IShape3D> shape;
    ColliderFilter filter{};
    ColliderMaterial material{};
    GameObject* owner = nullptr;
};

}  // namespace Spark
