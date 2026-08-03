#pragma once

#include "spark/physics/shapes/IShape2D.hpp"

namespace Spark {

class CircleShape2D final : public IShape2D {
public:
    CircleShape2D(float centerXIn, float centerYIn, float radiusIn) noexcept
            : centerX(centerXIn), centerY(centerYIn), radius(radiusIn) {}

    [[nodiscard]] ShapeType2D GetType() const noexcept override { return ShapeType2D::Circle; }
    [[nodiscard]] CollisionAabb2 GetBounds() const noexcept override;
    [[nodiscard]] float GetCenterX() const noexcept { return centerX; }
    [[nodiscard]] float GetCenterY() const noexcept { return centerY; }
    [[nodiscard]] float GetRadius() const noexcept { return radius; }

    void Translate(float deltaX, float deltaY) noexcept;

    [[nodiscard]] bool Overlaps(const IShape2D& other) const override;
    [[nodiscard]] bool OverlapsAabb(const CollisionAabb2& aabb) const override;
    [[nodiscard]] bool OverlapsCircle(float otherCenterX, float otherCenterY, float otherRadius) const override;
    [[nodiscard]] bool Raycast(const Ray2D& ray, float& outDistance) const override;
    [[nodiscard]] bool ComputeContact(const IShape2D& other, ContactManifold2D& out) const override;

private:
    float centerX = 0.0F;
    float centerY = 0.0F;
    float radius = 0.0F;
};

}  // namespace Spark
