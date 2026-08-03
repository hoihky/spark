#include "spark/physics/shapes/ShapeFactory3D.hpp"

#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/shapes/BoxShape3D.hpp"
#include "spark/physics/shapes/CapsuleShape3D.hpp"
#include "spark/physics/shapes/SphereShape3D.hpp"

namespace Spark {

UniquePtr<IShape3D> ShapeFactory3D::CreateBox(CollisionAabb3 aabb) {
    return UniquePtr<IShape3D>(new BoxShape3D(aabb));
}

UniquePtr<IShape3D> ShapeFactory3D::CreateSphere(Vector3 center, const float radius) {
    return UniquePtr<IShape3D>(new SphereShape3D(center, radius));
}

UniquePtr<IShape3D> ShapeFactory3D::CreateCapsule(CollisionCapsule3 capsule) {
    return UniquePtr<IShape3D>(new CapsuleShape3D(capsule));
}

UniquePtr<IShape3D> ShapeFactory3D::CreateFromStaticCollider(const StaticCollider3DSim& collider) {
    if (collider.shape == StaticCollider3DShape::Capsule) {
        return CreateCapsule(collider.capsule);
    }
    return CreateBox(collider.aabb);
}

UniquePtr<IShape3D> ShapeFactory3D::CreateFromBoxCollider(
        GameObject& owner,
        const BoxCollider3DComponent& collider) {
    CollisionAabb3 aabb{};
    ComputeBoxCollider3WorldAabb(owner, collider, aabb);
    return CreateBox(aabb);
}

UniquePtr<IShape3D> ShapeFactory3D::CreateFromSphereCollider(
        GameObject& owner,
        const SphereCollider3DComponent& collider) {
    Vector3 center{Vector3::Zero};
    float radius = 0.5F;
    ComputeSphereCollider3World(owner, collider, center, radius);
    return CreateSphere(center, radius);
}

UniquePtr<IShape3D> ShapeFactory3D::CreateFromCapsuleCollider(
        GameObject& owner,
        const CapsuleCollider3DComponent& collider) {
    CollisionCapsule3 capsule{};
    ComputeCapsuleCollider3World(owner, collider, capsule);
    return CreateCapsule(capsule);
}

}  // namespace Spark
