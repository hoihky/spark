#include "spark/physics/CharacterController2D.hpp"

#include "spark/core/Array.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CharacterController2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/OneWayPlatform2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/math/Constants.hpp"
#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/BroadPhase2D.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"
#include "spark/physics/shapes/ShapeType2D.hpp"
#include "spark/physics/SpatialHashGrid2D.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

struct ControllerBody2D {
    GameObject* object = nullptr;
    CharacterController2DComponent* controller = nullptr;
    Rigidbody2DComponent* rigidbody = nullptr;
    TransformComponent* transform = nullptr;
};

void CollectControllers(GameWorld& world, Array<ControllerBody2D>& out) noexcept {
    out.Clear();
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        auto* controller = object->GetComponent<CharacterController2DComponent>();
        auto* rigidbody = object->GetComponent<Rigidbody2DComponent>();
        auto* transform = object->GetComponent<TransformComponent>();
        if (controller == nullptr || rigidbody == nullptr || transform == nullptr) {
            return;
        }
        if (rigidbody->GetBodyType() != RigidbodyBodyType2D::Dynamic) {
            return;
        }
        if (object->GetComponent<BoxCollider2DComponent>() == nullptr &&
                object->GetComponent<CircleCollider2DComponent>() == nullptr) {
            return;
        }
        ControllerBody2D body{};
        body.object = object;
        body.controller = controller;
        body.rigidbody = rigidbody;
        body.transform = transform;
        out.PushBack(body);
    });
}

[[nodiscard]] float ComputeFeetY(GameObject& object) noexcept {
    if (auto* box = object.GetComponent<BoxCollider2DComponent>()) {
        CollisionAabb2 aabb{};
        ComputeBoxCollider2WorldAabb(object, *box, aabb);
        return aabb.minY;
    }
    if (auto* circle = object.GetComponent<CircleCollider2DComponent>()) {
        float cx = 0.0F;
        float cy = 0.0F;
        float cr = 0.0F;
        ComputeCircleCollider2World(object, *circle, cx, cy, cr);
        return cy - cr;
    }
    return object.GetWorldMatrix().TranslationVector().y;
}

[[nodiscard]] bool ProbeGrounded(
        GameObject& object,
        CharacterController2DComponent& controller,
        const Array<Collider2D>& colliders,
        SpatialHashGrid2D& broadPhase,
        Array<std::uint32_t>& scratch) noexcept {
    const float feetY = ComputeFeetY(object);
    const float probeY = feetY - controller.GetSkinWidth();
    const float probeX = object.GetWorldMatrix().TranslationVector().x;
    const float radius = 0.12F;
    const float slopeLimitCos = std::cos(controller.GetSlopeLimitDegrees() * (Pi / 180.0F));

    CollisionAabb2 query{};
    query.minX = probeX - radius;
    query.maxX = probeX + radius;
    query.minY = probeY - radius;
    query.maxY = probeY + radius;
    broadPhase.QueryUniquePayloadIndices(query, scratch);

    for (std::size_t i = 0; i < scratch.GetSize(); ++i) {
        const std::uint32_t idx = scratch[i];
        if (idx >= colliders.GetSize()) {
            continue;
        }
        const Collider2D& col = colliders[idx];
        if (col.IsTrigger()) {
            continue;
        }
        if (!ShouldResolveCharacterAgainstStatic2D(object, *object.GetComponent<Rigidbody2DComponent>(), col, feetY)) {
            continue;
        }
        if (!col.OverlapsCircle(probeX, probeY, radius)) {
            continue;
        }
        if (col.GetShapeType() == ShapeType2D::Box) {
            const CollisionAabb2 bounds = col.GetBounds();
            const float surfaceY = bounds.maxY;
            if (probeY <= surfaceY + controller.GetSkinWidth() + 0.02F && feetY >= surfaceY - 0.25F) {
                return true;
            }
            continue;
        }
        const StaticCollider2D snap = col.ToLegacySnapshot();
        if (snap.shape == StaticCollider2DShape::Circle) {
            const float ny = (probeY - snap.circleCy) / std::max(snap.circleR, 1.0e-4F);
            if (ny >= slopeLimitCos) {
                return true;
            }
            continue;
        }
        return true;
    }
    return false;
}

void SnapToGround(
        ControllerBody2D& body,
        const BroadPhase2D& broadPhase) noexcept {
    CharacterController2DComponent& controller = *body.controller;
    const float snapDistance = controller.GetSnapToGroundDistance();
    if (snapDistance <= 1.0e-6F) {
        return;
    }

    const float feetY = ComputeFeetY(*body.object);
    const float probeX = body.object->GetWorldMatrix().TranslationVector().x;
    PhysicsQueryFilter2D filter{};
    filter.hitSolids = true;
    filter.hitTriggers = false;

    PhysicsRaycastHit2D hit{};
    if (!RaycastStatics2D(broadPhase, probeX, feetY, 0.0F, -1.0F, snapDistance, filter, hit)) {
        return;
    }
    const Array<Collider2D>& colliders = broadPhase.GetColliders();
    if (hit.staticColliderIndex >= colliders.GetSize()) {
        return;
    }
    const Collider2D& col = colliders[hit.staticColliderIndex];
    if (!ShouldResolveCharacterAgainstStatic2D(
                *body.object, *body.rigidbody, col, feetY)) {
        return;
    }

    const float targetFeetY = hit.hitY;
    const float deltaY = targetFeetY - feetY;
    if (deltaY >= -controller.GetSkinWidth() && deltaY <= snapDistance) {
        Vector3 pos = body.transform->GetLocalTransform().translation;
        pos.y += deltaY;
        body.transform->SetTranslation(pos);
        Vector2 v = body.rigidbody->GetVelocity();
        if (v.y < 0.0F) {
            v.y = 0.0F;
            body.rigidbody->SetVelocity(v);
        }
    }
}

}  // namespace

bool ShouldResolveCharacterAgainstStatic2D(
        GameObject& dynamicObject,
        Rigidbody2DComponent& rigidbody,
        const Collider2D& staticCollider,
        const float dynamicFeetY) noexcept {
    const GameObject* staticOwner = staticCollider.GetOwner();
    if (staticOwner == nullptr || staticOwner->GetComponent<OneWayPlatform2DComponent>() == nullptr) {
        return true;
    }

    if (const auto* controller = dynamicObject.GetComponent<CharacterController2DComponent>()) {
        if (controller->GetDropThroughOneWay()) {
            return false;
        }
    }

    const CollisionAabb2 bounds = staticCollider.GetBounds();
    const float platformTop = bounds.maxY;
    constexpr float kPassThroughTolerance = 0.08F;

    if (dynamicFeetY < platformTop - kPassThroughTolerance) {
        return false;
    }

    if (rigidbody.GetVelocity().y > 0.5F && dynamicFeetY < platformTop) {
        return false;
    }

    return true;
}

void CharacterControllerWorld2D::Prepare(GameWorld& world, const FrameTiming& timing) {
    const float dt = timing.deltaTimeSeconds;
    if (dt <= 0.0F) {
        return;
    }

    Array<ControllerBody2D> controllers;
    CollectControllers(world, controllers);

    for (std::size_t i = 0; i < controllers.GetSize(); ++i) {
        ControllerBody2D& body = controllers[i];
        CharacterController2DComponent& controller = *body.controller;
        Rigidbody2DComponent& rb = *body.rigidbody;

        controller.wasGroundedLastFrame = controller.grounded;
        controller.grounded = rb.IsGrounded();

        if (controller.grounded) {
            controller.coyoteTimeRemaining = controller.GetCoyoteTimeSeconds();
        } else if (controller.coyoteTimeRemaining > 0.0F) {
            controller.coyoteTimeRemaining = std::max(0.0F, controller.coyoteTimeRemaining - dt);
        }

        if (controller.jumpRequested) {
            controller.jumpBufferRemaining = controller.GetJumpBufferSeconds();
            controller.jumpRequested = false;
        } else if (controller.jumpBufferRemaining > 0.0F) {
            controller.jumpBufferRemaining = std::max(0.0F, controller.jumpBufferRemaining - dt);
        }

        Vector2 velocity = rb.GetVelocity();
        velocity.x = controller.GetMoveInputX() * controller.GetMoveSpeed();

        const bool canJump =
                (controller.grounded || controller.coyoteTimeRemaining > 0.0F) &&
                controller.jumpBufferRemaining > 0.0F;
        if (canJump) {
            velocity.y = controller.GetJumpSpeed();
            controller.jumpBufferRemaining = 0.0F;
            controller.coyoteTimeRemaining = 0.0F;
            controller.grounded = false;
        }

        rb.SetVelocity(velocity);
        controller.dropThroughOneWay = false;
    }
}

void CharacterControllerWorld2D::Finalize(GameWorld& world, const FrameTiming& timing) {
    (void)timing;

    BroadPhase2D broadPhase;
    broadPhase.Rebuild(world, settings.broadPhaseCellSize);

    Array<std::uint32_t> scratch;
    const Array<Collider2D>& colliders = broadPhase.GetColliders();
    SpatialHashGrid2D& grid = broadPhase.GetGrid();
    Array<ControllerBody2D> controllers;
    CollectControllers(world, controllers);

    for (std::size_t i = 0; i < controllers.GetSize(); ++i) {
        ControllerBody2D& body = controllers[i];
        SnapToGround(body, broadPhase);
        body.controller->grounded = ProbeGrounded(*body.object, *body.controller, colliders, grid, scratch);
        body.rigidbody->SetGrounded(body.controller->grounded);
    }
}

void SimulateCharacterControllers2D(
        GameWorld& world,
        const FrameTiming& timing,
        const CharacterController2DSettings& settings) {
    CharacterControllerWorld2D controllerWorld(settings);
    controllerWorld.Prepare(world, timing);
    controllerWorld.Finalize(world, timing);
}

}  // namespace Spark
