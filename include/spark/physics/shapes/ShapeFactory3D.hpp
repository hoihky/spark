#pragma once

#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/shapes/IShape3D.hpp"

namespace Spark {

class BoxCollider3DComponent;
class CapsuleCollider3DComponent;
class GameObject;
class SphereCollider3DComponent;

/** Factory for ECS-free 3D shapes (Factory Method). */
class ShapeFactory3D {
public:
    [[nodiscard]] static UniquePtr<IShape3D> CreateBox(CollisionAabb3 aabb);
    [[nodiscard]] static UniquePtr<IShape3D> CreateSphere(Vector3 center, float radius);
    [[nodiscard]] static UniquePtr<IShape3D> CreateCapsule(CollisionCapsule3 capsule);
    [[nodiscard]] static UniquePtr<IShape3D> CreateFromStaticCollider(const StaticCollider3DSim& collider);

    [[nodiscard]] static UniquePtr<IShape3D> CreateFromBoxCollider(
            GameObject& owner,
            const BoxCollider3DComponent& collider);
    [[nodiscard]] static UniquePtr<IShape3D> CreateFromSphereCollider(
            GameObject& owner,
            const SphereCollider3DComponent& collider);
    [[nodiscard]] static UniquePtr<IShape3D> CreateFromCapsuleCollider(
            GameObject& owner,
            const CapsuleCollider3DComponent& collider);
};

}  // namespace Spark
