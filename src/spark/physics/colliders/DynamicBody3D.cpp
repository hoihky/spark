#include "spark/physics/colliders/DynamicBody3D.hpp"

#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void RefreshDynamicBody3D(DynamicBody3D& body) noexcept {
    if (body.obj == nullptr) {
        return;
    }
    if (body.sphere != nullptr) {
        body.collider.RefreshFromSphere(*body.obj, *body.sphere);
        return;
    }
    if (body.capsule != nullptr) {
        body.collider.RefreshFromCapsule(*body.obj, *body.capsule);
    }
}

void CollectDynamicBodies3D(GameWorld& world, Array<DynamicBody3D>& out) noexcept {
    out.Clear();
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        if (o->GetComponent<CharacterController3DComponent>() != nullptr) {
            return;
        }
        auto* rb = o->GetComponent<Rigidbody3DComponent>();
        auto* tr = o->GetComponent<TransformComponent>();
        if (rb == nullptr || tr == nullptr) {
            return;
        }
        if (rb->GetBodyType() != RigidbodyBodyType3D::Dynamic) {
            return;
        }

        auto* sphere = o->GetComponent<SphereCollider3DComponent>();
        auto* capsule = o->GetComponent<CapsuleCollider3DComponent>();
        if (sphere == nullptr && capsule == nullptr) {
            return;
        }

        DynamicBody3D body{};
        body.obj = o;
        body.rb = rb;
        body.tr = tr;
        body.sphere = sphere;
        body.capsule = (sphere == nullptr) ? capsule : nullptr;
        RefreshDynamicBody3D(body);
        out.PushBack(MoveTemp(body));
    });
}

}  // namespace Spark
