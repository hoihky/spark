#pragma once

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/shapes/IShape2D.hpp"

namespace Spark {

/**
 * Convex polygon in world space. Vertex count is in [3, kMaxStaticPolygonVertices].
 */
class ConvexPolygonShape2D final : public IShape2D {
public:
    ConvexPolygonShape2D() = default;
    ConvexPolygonShape2D(const StaticCollider2D& bakedPolygon);

    [[nodiscard]] ShapeType2D GetType() const noexcept override { return ShapeType2D::ConvexPolygon; }
    [[nodiscard]] CollisionAabb2 GetBounds() const noexcept override { return bounds; }
    [[nodiscard]] std::uint8_t GetVertexCount() const noexcept { return vertexCount; }
    [[nodiscard]] const StaticCollider2D& AsStaticColliderSnapshot() const noexcept { return snapshot; }

    [[nodiscard]] bool Overlaps(const IShape2D& other) const override;
    [[nodiscard]] bool OverlapsAabb(const CollisionAabb2& aabb) const override;
    [[nodiscard]] bool OverlapsCircle(float centerX, float centerY, float radius) const override;
    [[nodiscard]] bool Raycast(const Ray2D& ray, float& outDistance) const override;
    [[nodiscard]] bool ComputeContact(const IShape2D& other, ContactManifold2D& out) const override;

private:
    StaticCollider2D snapshot{};
    CollisionAabb2 bounds{};
    std::uint8_t vertexCount = 0;
};

}  // namespace Spark
