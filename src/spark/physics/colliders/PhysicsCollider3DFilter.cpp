#include "spark/physics/colliders/PhysicsCollider3DFilter.hpp"

#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/MeshCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsLayer3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

ColliderFilter PhysicsCollider3DFilter::Resolve(const GameObject& object) noexcept {
    if (const CharacterController3DComponent* controller = object.GetComponent<CharacterController3DComponent>()) {
        ColliderFilter filter{};
        filter.categoryBits = controller->GetCategoryBits();
        filter.maskBits = controller->GetMaskBits();
        return filter;
    }
    if (const BoxCollider3DComponent* box = object.GetComponent<BoxCollider3DComponent>()) {
        ColliderFilter filter{};
        filter.categoryBits = box->GetCategoryBits();
        filter.maskBits = box->GetMaskBits();
        filter.isTrigger = box->GetIsTrigger();
        return filter;
    }
    if (const SphereCollider3DComponent* sphere = object.GetComponent<SphereCollider3DComponent>()) {
        ColliderFilter filter{};
        filter.categoryBits = sphere->GetCategoryBits();
        filter.maskBits = sphere->GetMaskBits();
        filter.isTrigger = sphere->GetIsTrigger();
        return filter;
    }
    if (const CapsuleCollider3DComponent* capsule = object.GetComponent<CapsuleCollider3DComponent>()) {
        ColliderFilter filter{};
        filter.categoryBits = capsule->GetCategoryBits();
        filter.maskBits = capsule->GetMaskBits();
        filter.isTrigger = capsule->GetIsTrigger();
        return filter;
    }
    if (const MeshCollider3DComponent* mesh = object.GetComponent<MeshCollider3DComponent>()) {
        ColliderFilter filter{};
        filter.categoryBits = mesh->GetCategoryBits();
        filter.maskBits = mesh->GetMaskBits();
        filter.isTrigger = mesh->GetIsTrigger();
        return filter;
    }
    if (const PhysicsLayer3DComponent* layer = object.GetComponent<PhysicsLayer3DComponent>()) {
        return layer->GetFilter();
    }
    return ColliderFilter{};
}

}  // namespace Spark
