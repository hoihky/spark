#include "spark/physics/colliders/Collider3D.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/physics/shapes/BoxShape3D.hpp"
#include "spark/physics/shapes/CapsuleShape3D.hpp"
#include "spark/physics/shapes/ShapeFactory3D.hpp"
#include "spark/physics/shapes/SphereShape3D.hpp"

namespace Spark {

Collider3D Collider3D::Create(
        UniquePtr<IShape3D> shapeIn,
        ColliderFilter filterIn,
        ColliderMaterial materialIn,
        GameObject* ownerIn) noexcept {
    Collider3D collider{};
    collider.shape = MoveTemp(shapeIn);
    collider.filter = filterIn;
    collider.material = materialIn;
    collider.owner = ownerIn;
    return collider;
}

Collider3D Collider3D::FromLegacySnapshot(const StaticCollider3DSim& snapshot) {
    UniquePtr<IShape3D> shape = ShapeFactory3D::CreateFromStaticCollider(snapshot);
    Collider3D collider{};
    collider.shape = MoveTemp(shape);
    collider.filter = ColliderFilter::FromStaticCollider3D(snapshot);
    collider.material = ColliderMaterial::FromStaticCollider3D(snapshot);
    collider.owner = nullptr;
    return collider;
}

StaticCollider3DSim Collider3D::ToLegacySnapshot() const {
    StaticCollider3DSim snapshot{};
    if (!shape) {
        return snapshot;
    }

    snapshot.hasMaterial = material.isDefined;
    snapshot.restitution = material.restitution;
    snapshot.staticFriction = material.staticFriction;
    snapshot.dynamicFriction = material.dynamicFriction;
    snapshot.aabb = shape->GetBounds();

    const ShapeType3D type = shape->GetType();
    if (type == ShapeType3D::Box) {
        snapshot.shape = StaticCollider3DShape::Box;
        snapshot.aabb = static_cast<const BoxShape3D&>(*shape).GetAabb();
        return snapshot;
    }
    if (type == ShapeType3D::Sphere) {
        const SphereShape3D& sphere = static_cast<const SphereShape3D&>(*shape);
        snapshot.shape = StaticCollider3DShape::Box;
        snapshot.aabb = sphere.GetBounds();
        return snapshot;
    }
    if (type == ShapeType3D::Capsule) {
        snapshot.shape = StaticCollider3DShape::Capsule;
        snapshot.capsule = static_cast<const CapsuleShape3D&>(*shape).GetCapsule();
        snapshot.aabb = static_cast<const CapsuleShape3D&>(*shape).GetBounds();
    }
    return snapshot;
}

ShapeType3D Collider3D::GetShapeType() const noexcept {
    return shape ? shape->GetType() : ShapeType3D::Box;
}

CollisionAabb3 Collider3D::GetBounds() const noexcept {
    return shape ? shape->GetBounds() : CollisionAabb3{};
}

bool Collider3D::OverlapsAabb(const CollisionAabb3& aabb) const {
    return shape ? shape->OverlapsAabb(aabb) : false;
}

bool Collider3D::OverlapsSphere(const Vector3& center, const float radius) const {
    return shape ? shape->OverlapsSphere(center, radius) : false;
}

}  // namespace Spark
