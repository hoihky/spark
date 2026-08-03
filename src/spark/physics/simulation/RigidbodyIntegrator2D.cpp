#include "spark/physics/simulation/RigidbodyIntegrator2D.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Vector2.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/scene/GameWorld.hpp"

namespace Spark {

void RigidbodyIntegrator2D::Integrate(
        GameWorld& world,
        const float deltaTimeSeconds,
        const PhysicsWorld2DSettings& settings) noexcept {
    world.ForEachActiveGameObject([&](GameObject* o) {
        if (o == nullptr) {
            return;
        }
        Rigidbody2DComponent* rb = o->GetComponent<Rigidbody2DComponent>();
        TransformComponent* tr = o->GetComponent<TransformComponent>();
        if (rb == nullptr || tr == nullptr) {
            return;
        }
        if (rb->GetBodyType() != RigidbodyBodyType2D::Dynamic) {
            return;
        }
        CircleCollider2DComponent* circleCol = o->GetComponent<CircleCollider2DComponent>();
        BoxCollider2DComponent* boxCol = o->GetComponent<BoxCollider2DComponent>();
        if (circleCol == nullptr && boxCol == nullptr) {
            return;
        }

        Vector2 v = rb->GetVelocity();
        v.y += settings.gravityY * rb->GetGravityScale() * deltaTimeSeconds;
        if (v.y < -settings.maxFallSpeed) {
            v.y = -settings.maxFallSpeed;
        }
        rb->SetVelocity(v);

        Vector3 pos = tr->GetLocalTransform().translation;
        pos.x += v.x * deltaTimeSeconds;
        pos.y += v.y * deltaTimeSeconds;
        tr->SetTranslation(pos);
    });
}

}  // namespace Spark
