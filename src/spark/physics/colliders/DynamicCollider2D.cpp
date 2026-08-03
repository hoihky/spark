#include "spark/physics/colliders/DynamicCollider2D.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/shapes/BoxShape2D.hpp"
#include "spark/physics/shapes/CircleShape2D.hpp"
#include "spark/physics/shapes/NarrowPhase2D.hpp"
#include "spark/physics/shapes/ShapeFactory2D.hpp"

namespace Spark {

DynamicCollider2D DynamicCollider2D::FromLegacySnapshot(const DynamicCollider2DSim& snapshot) {
    DynamicCollider2D collider{};
    if (snapshot.shape == DynamicCollider2DShape::Circle) {
        collider.shape = ShapeFactory2D::CreateCircle(snapshot.circleCx, snapshot.circleCy, snapshot.circleR);
    } else {
        collider.shape = ShapeFactory2D::CreateBox(snapshot.aabb);
    }
    return collider;
}

DynamicCollider2DSim DynamicCollider2D::ToLegacySnapshot() const {
    DynamicCollider2DSim snapshot{};
    if (!shape) {
        return snapshot;
    }
    snapshot.aabb = shape->GetBounds();
    const ShapeType2D type = shape->GetType();
    if (type == ShapeType2D::Circle) {
        const CircleShape2D& circle = static_cast<const CircleShape2D&>(*shape);
        snapshot.shape = DynamicCollider2DShape::Circle;
        snapshot.circleCx = circle.GetCenterX();
        snapshot.circleCy = circle.GetCenterY();
        snapshot.circleR = circle.GetRadius();
        return snapshot;
    }
    snapshot.shape = DynamicCollider2DShape::Box;
    snapshot.aabb = static_cast<const BoxShape2D&>(*shape).GetAabb();
    return snapshot;
}

void DynamicCollider2D::RefreshFromBox(GameObject& owner, const BoxCollider2DComponent& collider) {
    shape = ShapeFactory2D::CreateFromBoxCollider(owner, collider);
    filter.categoryBits = collider.GetCategoryBits();
    filter.maskBits = collider.GetMaskBits();
    filter.isTrigger = collider.GetIsTrigger();
}

void DynamicCollider2D::RefreshFromCircle(GameObject& owner, const CircleCollider2DComponent& collider) {
    shape = ShapeFactory2D::CreateFromCircleCollider(owner, collider);
    filter.categoryBits = collider.GetCategoryBits();
    filter.maskBits = collider.GetMaskBits();
    filter.isTrigger = collider.GetIsTrigger();
}

ShapeType2D DynamicCollider2D::GetShapeType() const noexcept {
    return shape ? shape->GetType() : ShapeType2D::Box;
}

CollisionAabb2 DynamicCollider2D::GetBounds() const noexcept {
    return shape ? shape->GetBounds() : CollisionAabb2{};
}

void DynamicCollider2D::Translate(const float deltaX, const float deltaY) noexcept {
    if (!shape) {
        return;
    }
    if (shape->GetType() == ShapeType2D::Circle) {
        static_cast<CircleShape2D&>(*shape).Translate(deltaX, deltaY);
        return;
    }
    static_cast<BoxShape2D&>(*shape).Translate(deltaX, deltaY);
}

bool DynamicCollider2D::Overlaps(const DynamicCollider2D& other) const {
    if (!shape || !other.shape) {
        return false;
    }
    return NarrowPhase2D::Overlap(*shape, *other.shape);
}

bool DynamicCollider2D::OverlapsStatic(const Collider2D& staticCollider) const {
    if (!shape || !staticCollider.IsValid()) {
        return false;
    }
    return NarrowPhase2D::Overlap(*shape, staticCollider.GetShape());
}

bool DynamicCollider2D::OverlapsAabb(const CollisionAabb2& aabb) const {
    return shape ? shape->OverlapsAabb(aabb) : false;
}

bool DynamicCollider2D::OverlapsCircle(const float centerX, const float centerY, const float radius) const {
    return shape ? shape->OverlapsCircle(centerX, centerY, radius) : false;
}

}  // namespace Spark
