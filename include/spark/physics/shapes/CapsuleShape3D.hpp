#pragma once

#include "spark/physics/shapes/IShape3D.hpp"

namespace Spark {

class CapsuleShape3D final : public IShape3D {
public:
    explicit CapsuleShape3D(CollisionCapsule3 capsuleIn) noexcept : capsule(capsuleIn) {}

    [[nodiscard]] ShapeType3D GetType() const noexcept override { return ShapeType3D::Capsule; }
    [[nodiscard]] CollisionAabb3 GetBounds() const noexcept override;
    [[nodiscard]] const CollisionCapsule3& GetCapsule() const noexcept { return capsule; }

    void Translate(const Vector3& delta) noexcept;

    [[nodiscard]] bool Overlaps(const IShape3D& other) const override;
    [[nodiscard]] bool OverlapsAabb(const CollisionAabb3& aabb) const override;
    [[nodiscard]] bool OverlapsSphere(const Vector3& center, float radius) const override;
    [[nodiscard]] bool Raycast(const Ray3D& ray, float& outDistance) const override;
    [[nodiscard]] bool ComputeContact(const IShape3D& other, ContactManifold3D& out) const override;

private:
    CollisionCapsule3 capsule{};
};

}  // namespace Spark
