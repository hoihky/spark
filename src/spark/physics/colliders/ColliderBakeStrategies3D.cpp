#include "spark/physics/colliders/ColliderBakePipeline3D.hpp"

#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/MeshCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/colliders/ColliderBake3D.hpp"
#include "spark/physics/colliders/ColliderBakeHelpers3D.hpp"
#include "spark/physics/colliders/IColliderBakeStrategy3D.hpp"
#include "spark/physics/MeshCollider3D.hpp"
#include "spark/physics/shapes/ShapeFactory3D.hpp"

namespace Spark {

namespace {

[[nodiscard]] bool ContributesAnyStaticCollider3D(GameObject& object) noexcept {
    if (object.GetComponent<CharacterController3DComponent>() != nullptr) {
        return false;
    }
    const bool hasBox = object.GetComponent<BoxCollider3DComponent>() != nullptr;
    const bool hasCapsule = object.GetComponent<CapsuleCollider3DComponent>() != nullptr;
    const bool hasMesh = object.GetComponent<MeshCollider3DComponent>() != nullptr;
    if (!hasBox && !hasCapsule && !hasMesh) {
        return false;
    }
    const Rigidbody3DComponent* rb = object.GetComponent<Rigidbody3DComponent>();
    if (rb == nullptr) {
        return true;
    }
    return rb->GetBodyType() != RigidbodyBodyType3D::Dynamic;
}

class BoxColliderBakeStrategy3D final : public IColliderBakeStrategy3D {
public:
    [[nodiscard]] bool Contributes(GameObject& object) const noexcept override {
        if (object.GetComponent<BoxCollider3DComponent>() == nullptr) {
            return false;
        }
        return ContributesAnyStaticCollider3D(object);
    }

    void Bake(GameObject& object, ColliderBakeContext3D& context) const override {
        const BoxCollider3DComponent* box = object.GetComponent<BoxCollider3DComponent>();
        if (box == nullptr) {
            return;
        }
        UniquePtr<IShape3D> shape = ShapeFactory3D::CreateFromBoxCollider(object, *box);
        PushCollider3D(
                context.colliders,
                context.grid,
                Collider3D::Create(MoveTemp(shape), ColliderFilter{}, MaterialFromGameObject3D(object), &object));
    }
};

class CapsuleColliderBakeStrategy3D final : public IColliderBakeStrategy3D {
public:
    [[nodiscard]] bool Contributes(GameObject& object) const noexcept override {
        if (object.GetComponent<CapsuleCollider3DComponent>() == nullptr) {
            return false;
        }
        return ContributesAnyStaticCollider3D(object);
    }

    void Bake(GameObject& object, ColliderBakeContext3D& context) const override {
        const CapsuleCollider3DComponent* capsule = object.GetComponent<CapsuleCollider3DComponent>();
        if (capsule == nullptr) {
            return;
        }
        UniquePtr<IShape3D> shape = ShapeFactory3D::CreateFromCapsuleCollider(object, *capsule);
        PushCollider3D(
                context.colliders,
                context.grid,
                Collider3D::Create(MoveTemp(shape), ColliderFilter{}, MaterialFromGameObject3D(object), &object));
    }
};

class MeshColliderBakeStrategy3D final : public IColliderBakeStrategy3D {
public:
    [[nodiscard]] bool Contributes(GameObject& object) const noexcept override {
        return ContributesMeshCollider3DStatic(object);
    }

    void Bake(GameObject& object, ColliderBakeContext3D& context) const override {
        const MeshCollider3DComponent* meshCol = object.GetComponent<MeshCollider3DComponent>();
        if (meshCol == nullptr) {
            return;
        }
        CollisionAabb3 aabb{};
        ComputeMeshCollider3WorldAabb(object, *meshCol, aabb);
        UniquePtr<IShape3D> shape = ShapeFactory3D::CreateBox(aabb);
        PushCollider3D(
                context.colliders,
                context.grid,
                Collider3D::Create(MoveTemp(shape), ColliderFilter{}, MaterialFromGameObject3D(object), &object));
    }
};

}  // namespace

void RegisterDefaultColliderBakeStrategies3D(ColliderBakePipeline3D& pipeline) {
    pipeline.RegisterStrategy(UniquePtr<IColliderBakeStrategy3D>(new BoxColliderBakeStrategy3D()));
    pipeline.RegisterStrategy(UniquePtr<IColliderBakeStrategy3D>(new CapsuleColliderBakeStrategy3D()));
    pipeline.RegisterStrategy(UniquePtr<IColliderBakeStrategy3D>(new MeshColliderBakeStrategy3D()));
}

}  // namespace Spark
