#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/components/physics/3d/CapsuleCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/colliders/DynamicCollider3D.hpp"

namespace Spark {

class GameWorld;

/** ECS handles for one integrated dynamic 3D rigidbody collider. */
struct DynamicBody3D {
    GameObject* obj = nullptr;
    Rigidbody3DComponent* rb = nullptr;
    TransformComponent* tr = nullptr;
    SphereCollider3DComponent* sphere = nullptr;
    CapsuleCollider3DComponent* capsule = nullptr;
    DynamicCollider3D collider{};
};

void RefreshDynamicBody3D(DynamicBody3D& body) noexcept;

void CollectDynamicBodies3D(GameWorld& world, Array<DynamicBody3D>& out) noexcept;

}  // namespace Spark
