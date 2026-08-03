#pragma once

#include "spark/physics/shapes/IShape3D.hpp"

namespace Spark {

class SphereShape3D final : public IShape3D {
public:
    SphereShape3D(Vector3 centerIn, float radiusIn) noexcept : center(centerIn), radius(radiusIn) {}

    [[nodiscard]] ShapeType3D GetType() const noexcept override { return ShapeType3D::Sphere; }
    [[nodiscard]] CollisionAabb3 GetBounds() const noexcept override;
    [[nodiscard]] const Vector3& GetCenter() const noexcept { return center; }
    [[nodiscard]] float GetRadius() const noexcept { return radius; }

    void Translate(const Vector3& delta) noexcept;

    [[nodiscard]] bool Overlaps(const IShape3D& other) const override;
    [[nodiscard]] bool OverlapsAabb(const CollisionAabb3& aabb) const override;
    [[nodiscard]] bool OverlapsSphere(const Vector3& otherCenter, float otherRadius) const override;
    [[nodiscard]] bool Raycast(const Ray3D& ray, float& outDistance) const override;
    [[nodiscard]] bool ComputeContact(const IShape3D& other, ContactManifold3D& out) const override;

private:
    Vector3 center{Vector3::Zero};
    float radius = 0.5F;
};

}  // namespace Spark
