#include "spark/physics/shapes/ShapeFactory2D.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/PolygonCollider2DComponent.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/shapes/BoxShape2D.hpp"
#include "spark/physics/shapes/CircleShape2D.hpp"
#include "spark/physics/shapes/ConvexPolygonShape2D.hpp"

namespace Spark {

UniquePtr<IShape2D> ShapeFactory2D::CreateBox(CollisionAabb2 aabb) {
    return UniquePtr<IShape2D>(new BoxShape2D(aabb));
}

UniquePtr<IShape2D> ShapeFactory2D::CreateCircle(const float centerX, const float centerY, const float radius) {
    return UniquePtr<IShape2D>(new CircleShape2D(centerX, centerY, radius));
}

UniquePtr<IShape2D> ShapeFactory2D::CreateConvexPolygon(const StaticCollider2D& bakedPolygon) {
    return UniquePtr<IShape2D>(new ConvexPolygonShape2D(bakedPolygon));
}

UniquePtr<IShape2D> ShapeFactory2D::CreateFromStaticCollider(const StaticCollider2D& collider) {
    if (collider.shape == StaticCollider2DShape::Circle) {
        return CreateCircle(collider.circleCx, collider.circleCy, collider.circleR);
    }
    if (collider.shape == StaticCollider2DShape::ConvexPolygon) {
        return CreateConvexPolygon(collider);
    }
    return CreateBox(collider.aabb);
}

UniquePtr<IShape2D> ShapeFactory2D::CreateFromBoxCollider(
        GameObject& owner,
        const BoxCollider2DComponent& collider) {
    CollisionAabb2 aabb{};
    ComputeBoxCollider2WorldAabb(owner, collider, aabb);
    return CreateBox(aabb);
}

UniquePtr<IShape2D> ShapeFactory2D::CreateFromCircleCollider(
        GameObject& owner,
        const CircleCollider2DComponent& collider) {
    float cx = 0.0F;
    float cy = 0.0F;
    float r = 0.0F;
    ComputeCircleCollider2World(owner, collider, cx, cy, r);
    return CreateCircle(cx, cy, r);
}

UniquePtr<IShape2D> ShapeFactory2D::CreateFromPolygonCollider(
        GameObject& owner,
        const PolygonCollider2DComponent& collider) {
    StaticCollider2D baked{};
    baked.shape = StaticCollider2DShape::ConvexPolygon;
    ComputePolygonCollider2DWorld(owner, collider, baked);
    return CreateConvexPolygon(baked);
}

}  // namespace Spark
