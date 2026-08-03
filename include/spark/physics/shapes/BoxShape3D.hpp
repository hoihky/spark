#pragma once

#include "spark/physics/shapes/IShape3D.hpp"

namespace Spark {

class BoxShape3D final : public IShape3D {
public:
    explicit BoxShape3D(CollisionAabb3 aabbIn) noexcept : aabb(aabbIn) {}

    [[nodiscard]] ShapeType3D GetType() const noexcept override { return ShapeType3D::Box; }
    [[nodiscard]] CollisionAabb3 GetBounds() const noexcept override { return aabb; }
    [[nodiscard]] const CollisionAabb3& GetAabb() const noexcept { return aabb; }

    [[nodiscard]] bool Overlaps(const IShape3D& other) const override;
    [[nodiscard]] bool OverlapsAabb(const CollisionAabb3& otherAabb) const override;
    [[nodiscard]] bool OverlapsSphere(const Vector3& center, float radius) const override;
    [[nodiscard]] bool Raycast(const Ray3D& ray, float& outDistance) const override;
    [[nodiscard]] bool ComputeContact(const IShape3D& other, ContactManifold3D& out) const override;

private:
    CollisionAabb3 aabb{};
};

}  // namespace Spark
