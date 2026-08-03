#include "spark/physics/colliders/DynamicBody2D.hpp"

#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void RefreshDynamicBody2D(DynamicBody2D& body) noexcept {
    if (body.object == nullptr) {
        return;
    }
    if (body.circle != nullptr) {
        body.collider.RefreshFromCircle(*body.object, *body.circle);
        return;
    }
    if (body.box != nullptr) {
        body.collider.RefreshFromBox(*body.object, *body.box);
    }
}

void CollectDynamicBodies2D(GameWorld& world, Array<DynamicBody2D>& out) {
    out.Clear();
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        Rigidbody2DComponent* rb = object->GetComponent<Rigidbody2DComponent>();
        TransformComponent* tr = object->GetComponent<TransformComponent>();
        if (rb == nullptr || tr == nullptr) {
            return;
        }
        if (rb->GetBodyType() != RigidbodyBodyType2D::Dynamic) {
            return;
        }
        BoxCollider2DComponent* box = object->GetComponent<BoxCollider2DComponent>();
        CircleCollider2DComponent* circle = object->GetComponent<CircleCollider2DComponent>();
        if (box == nullptr && circle == nullptr) {
            return;
        }
        DynamicBody2D body{};
        body.object = object;
        body.transform = tr;
        body.rb = rb;
        body.box = box;
        body.circle = circle;
        RefreshDynamicBody2D(body);
        out.PushBack(MoveTemp(body));
    });
}

}  // namespace Spark
