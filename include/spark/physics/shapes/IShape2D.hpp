#pragma once

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/core/ContactManifold.hpp"
#include "spark/physics/core/Ray.hpp"
#include "spark/physics/shapes/ShapeType2D.hpp"

namespace Spark {

/**
 * Polymorphic 2D collision shape. Concrete types live in <c>spark/physics/shapes/</c>.
 * Own instances through <c>UniquePtr&lt;IShape2D&gt;</c> (see <c>ShapeFactory2D</c>).
 */
class IShape2D {
public:
    virtual ~IShape2D() = default;

    [[nodiscard]] virtual ShapeType2D GetType() const noexcept = 0;
    [[nodiscard]] virtual CollisionAabb2 GetBounds() const noexcept = 0;

    [[nodiscard]] virtual bool Overlaps(const IShape2D& other) const = 0;
    [[nodiscard]] virtual bool OverlapsAabb(const CollisionAabb2& aabb) const = 0;
    [[nodiscard]] virtual bool OverlapsCircle(float centerX, float centerY, float radius) const = 0;
    [[nodiscard]] virtual bool Raycast(const Ray2D& ray, float& outDistance) const = 0;
    [[nodiscard]] virtual bool ComputeContact(const IShape2D& other, ContactManifold2D& out) const = 0;
};

}  // namespace Spark
