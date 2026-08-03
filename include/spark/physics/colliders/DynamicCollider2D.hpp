#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/core/ColliderFilter.hpp"
#include "spark/physics/shapes/IShape2D.hpp"
#include "spark/physics/shapes/ShapeType2D.hpp"

namespace Spark {

class BoxCollider2DComponent;
class CircleCollider2DComponent;
class Collider2D;
class GameObject;

/** Legacy runtime tag for a dynamic 2D collider snapshot. */
enum class DynamicCollider2DShape : std::uint8_t {
    Box = 0,
    Circle = 1,
};

/** Legacy POD snapshot used during migration (Phase 4). */
struct DynamicCollider2DSim {
    DynamicCollider2DShape shape = DynamicCollider2DShape::Box;
    CollisionAabb2 aabb{};
    float circleCx = 0.0F;
    float circleCy = 0.0F;
    float circleR = 0.0F;
};

/**
 * Object-oriented dynamic 2D collider: owns an <c>IShape2D</c> plus filter bits from the ECS component.
 * Refreshed each simulation/query step from the owning transform + collider component.
 */
class DynamicCollider2D {
public:
    DynamicCollider2D() = default;
    DynamicCollider2D(DynamicCollider2D&&) noexcept = default;
    DynamicCollider2D& operator=(DynamicCollider2D&&) noexcept = default;
    DynamicCollider2D(const DynamicCollider2D&) = delete;
    DynamicCollider2D& operator=(const DynamicCollider2D&) = delete;

    static DynamicCollider2D FromLegacySnapshot(const DynamicCollider2DSim& snapshot);

    [[nodiscard]] DynamicCollider2DSim ToLegacySnapshot() const;

    void RefreshFromBox(GameObject& owner, const BoxCollider2DComponent& collider);
    void RefreshFromCircle(GameObject& owner, const CircleCollider2DComponent& collider);

    [[nodiscard]] bool IsValid() const noexcept { return shape != nullptr; }
    [[nodiscard]] const IShape2D& GetShape() const noexcept { return *shape; }
    [[nodiscard]] ShapeType2D GetShapeType() const noexcept;
    [[nodiscard]] CollisionAabb2 GetBounds() const noexcept;

    [[nodiscard]] const ColliderFilter& GetFilter() const noexcept { return filter; }

    void Translate(float deltaX, float deltaY) noexcept;

    [[nodiscard]] bool Overlaps(const DynamicCollider2D& other) const;
    [[nodiscard]] bool OverlapsStatic(const Collider2D& staticCollider) const;
    [[nodiscard]] bool OverlapsAabb(const CollisionAabb2& aabb) const;
    [[nodiscard]] bool OverlapsCircle(float centerX, float centerY, float radius) const;

private:
    UniquePtr<IShape2D> shape;
    ColliderFilter filter{};
};

}  // namespace Spark
