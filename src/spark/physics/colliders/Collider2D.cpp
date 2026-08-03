#include "spark/physics/colliders/Collider2D.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/physics/shapes/BoxShape2D.hpp"
#include "spark/physics/shapes/CircleShape2D.hpp"
#include "spark/physics/shapes/ConvexPolygonShape2D.hpp"
#include "spark/physics/shapes/ShapeFactory2D.hpp"

namespace Spark {

Collider2D Collider2D::Create(
        UniquePtr<IShape2D> shapeIn,
        ColliderFilter filterIn,
        ColliderMaterial materialIn,
        GameObject* ownerIn) noexcept {
    Collider2D collider{};
    collider.shape = MoveTemp(shapeIn);
    collider.filter = filterIn;
    collider.material = materialIn;
    collider.owner = ownerIn;
    return collider;
}

Collider2D Collider2D::FromLegacySnapshot(const StaticCollider2D& snapshot) {
    UniquePtr<IShape2D> shape = ShapeFactory2D::CreateFromStaticCollider(snapshot);
    Collider2D collider{};
    collider.shape = MoveTemp(shape);
    collider.filter = ColliderFilter::FromStaticCollider2D(snapshot);
    collider.material = ColliderMaterial::FromStaticCollider2D(snapshot);
    collider.owner = snapshot.owner;
    return collider;
}

StaticCollider2D Collider2D::ToLegacySnapshot() const {
    StaticCollider2D snapshot{};
    if (!shape) {
        return snapshot;
    }

    snapshot.owner = owner;
    snapshot.categoryBits = filter.categoryBits;
    snapshot.maskBits = filter.maskBits;
    snapshot.isTrigger = filter.isTrigger;
    snapshot.hasMaterial = material.isDefined;
    snapshot.restitution = material.restitution;
    snapshot.dynamicFriction = material.dynamicFriction;
    snapshot.aabb = shape->GetBounds();

    const ShapeType2D type = shape->GetType();
    if (type == ShapeType2D::Box) {
        snapshot.shape = StaticCollider2DShape::Box;
        snapshot.aabb = static_cast<const BoxShape2D&>(*shape).GetAabb();
        return snapshot;
    }
    if (type == ShapeType2D::Circle) {
        const CircleShape2D& circle = static_cast<const CircleShape2D&>(*shape);
        snapshot.shape = StaticCollider2DShape::Circle;
        snapshot.circleCx = circle.GetCenterX();
        snapshot.circleCy = circle.GetCenterY();
        snapshot.circleR = circle.GetRadius();
        return snapshot;
    }
    if (type == ShapeType2D::ConvexPolygon) {
        return static_cast<const ConvexPolygonShape2D&>(*shape).AsStaticColliderSnapshot();
    }
    return snapshot;
}

ShapeType2D Collider2D::GetShapeType() const noexcept {
    return shape ? shape->GetType() : ShapeType2D::Box;
}

CollisionAabb2 Collider2D::GetBounds() const noexcept {
    return shape ? shape->GetBounds() : CollisionAabb2{};
}

bool Collider2D::OverlapsAabb(const CollisionAabb2& aabb) const {
    return shape ? shape->OverlapsAabb(aabb) : false;
}

bool Collider2D::OverlapsCircle(const float centerX, const float centerY, const float radius) const {
    return shape ? shape->OverlapsCircle(centerX, centerY, radius) : false;
}

}  // namespace Spark
