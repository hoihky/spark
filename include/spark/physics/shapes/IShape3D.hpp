#pragma once

#include "spark/physics/Collision3D.hpp"
#include "spark/physics/core/ContactManifold.hpp"
#include "spark/physics/core/Ray.hpp"
#include "spark/physics/shapes/ShapeType3D.hpp"

namespace Spark {

/**
 * Polymorphic 3D collision shape. Concrete types live in <c>spark/physics/shapes/</c>.
 */
class IShape3D {
public:
    virtual ~IShape3D() = default;

    [[nodiscard]] virtual ShapeType3D GetType() const noexcept = 0;
    [[nodiscard]] virtual CollisionAabb3 GetBounds() const noexcept = 0;

    [[nodiscard]] virtual bool Overlaps(const IShape3D& other) const = 0;
    [[nodiscard]] virtual bool OverlapsAabb(const CollisionAabb3& aabb) const = 0;
    [[nodiscard]] virtual bool OverlapsSphere(const Vector3& center, float radius) const = 0;
    [[nodiscard]] virtual bool Raycast(const Ray3D& ray, float& outDistance) const = 0;
    [[nodiscard]] virtual bool ComputeContact(const IShape3D& other, ContactManifold3D& out) const = 0;
};

}  // namespace Spark
