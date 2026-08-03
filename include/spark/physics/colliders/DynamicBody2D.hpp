#pragma once

#include "spark/core/Array.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/physics/colliders/DynamicCollider2D.hpp"

namespace Spark {

class GameWorld;

/** ECS handles for one integrated dynamic 2D rigidbody collider. */
struct DynamicBody2D {
    GameObject* object = nullptr;
    TransformComponent* transform = nullptr;
    Rigidbody2DComponent* rb = nullptr;
    BoxCollider2DComponent* box = nullptr;
    CircleCollider2DComponent* circle = nullptr;
    DynamicCollider2D collider{};
};

/** Rebuilds <c>body.collider</c> from the attached box or circle component. */
void RefreshDynamicBody2D(DynamicBody2D& body) noexcept;

/** Appends every active dynamic rigidbody with a box or circle collider. */
void CollectDynamicBodies2D(GameWorld& world, Array<DynamicBody2D>& out);

}  // namespace Spark
