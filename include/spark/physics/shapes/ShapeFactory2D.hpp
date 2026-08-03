#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/shapes/IShape2D.hpp"

namespace Spark {

class BoxCollider2DComponent;
class CircleCollider2DComponent;
class GameObject;
class PolygonCollider2DComponent;

/** Factory for ECS-free 2D shapes (Factory Method). */
class ShapeFactory2D {
public:
    [[nodiscard]] static UniquePtr<IShape2D> CreateBox(CollisionAabb2 aabb);
    [[nodiscard]] static UniquePtr<IShape2D> CreateCircle(float centerX, float centerY, float radius);
    [[nodiscard]] static UniquePtr<IShape2D> CreateConvexPolygon(const StaticCollider2D& bakedPolygon);
    [[nodiscard]] static UniquePtr<IShape2D> CreateFromStaticCollider(const StaticCollider2D& collider);

    [[nodiscard]] static UniquePtr<IShape2D> CreateFromBoxCollider(
            GameObject& owner,
            const BoxCollider2DComponent& collider);
    [[nodiscard]] static UniquePtr<IShape2D> CreateFromCircleCollider(
            GameObject& owner,
            const CircleCollider2DComponent& collider);
    [[nodiscard]] static UniquePtr<IShape2D> CreateFromPolygonCollider(
            GameObject& owner,
            const PolygonCollider2DComponent& collider);
};

}  // namespace Spark
