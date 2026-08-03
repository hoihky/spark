#pragma once

#include "spark/physics/shapes/IShape2D.hpp"

namespace Spark {

class BoxShape2D final : public IShape2D {
public:
    explicit BoxShape2D(CollisionAabb2 aabbIn) noexcept : aabb(aabbIn) {}

    [[nodiscard]] ShapeType2D GetType() const noexcept override { return ShapeType2D::Box; }
    [[nodiscard]] CollisionAabb2 GetBounds() const noexcept override { return aabb; }
    [[nodiscard]] const CollisionAabb2& GetAabb() const noexcept { return aabb; }

    void Translate(float deltaX, float deltaY) noexcept;

    [[nodiscard]] bool Overlaps(const IShape2D& other) const override;
    [[nodiscard]] bool OverlapsAabb(const CollisionAabb2& otherAabb) const override;
    [[nodiscard]] bool OverlapsCircle(float centerX, float centerY, float radius) const override;
    [[nodiscard]] bool Raycast(const Ray2D& ray, float& outDistance) const override;
    [[nodiscard]] bool ComputeContact(const IShape2D& other, ContactManifold2D& out) const override;

private:
    CollisionAabb2 aabb{};
};

}  // namespace Spark
