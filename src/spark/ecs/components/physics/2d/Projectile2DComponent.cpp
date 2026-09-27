#include "spark/ecs/components/physics/2d/Projectile2DComponent.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/Hurtbox2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/CombatDamage2D.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

namespace {

bool ArrayContainsObject(const Array<GameObject*>& list, const GameObject* object) noexcept {
    for (std::size_t i = 0; i < list.GetSize(); ++i) {
        if (list[i] == object) {
            return true;
        }
    }
    return false;
}

}  // namespace

void Projectile2DComponent::SetRadius(const float value) noexcept {
    radius = std::max(0.01F, value);
}

void Projectile2DComponent::SetHalfExtents(const Vector2& value) noexcept {
    halfExtents.x = std::max(0.01F, value.x);
    halfExtents.y = std::max(0.01F, value.y);
}

void Projectile2DComponent::Activate(
        const Vector2& worldPosition,
        const Vector2& velocityIn,
        GameObject* instigatorIn) noexcept {
    active = true;
    ageSeconds = 0.0F;
    velocity = velocityIn;
    instigator = instigatorIn;
    hitVictims.Clear();
    pendingDestroyOwner = false;

    if (GameObject* owner = GetOwner()) {
        if (TransformComponent* tr = owner->GetComponent<TransformComponent>()) {
            const Vector3 pos = tr->GetLocalTransform().translation;
            tr->SetTranslation({worldPosition.x, worldPosition.y, pos.z});
        }
    }
}

void Projectile2DComponent::Deactivate() noexcept {
    active = false;
    velocity = Vector2::Zero;
    ageSeconds = 0.0F;
    hitVictims.Clear();
}

void Projectile2DComponent::MarkDeferredDestroy(GameObject& owner) noexcept {
    if (destroyOwnerOnDeactivate) {
        pendingDestroyOwner = true;
    }
    Deactivate();
    if (!destroyOwnerOnDeactivate) {
        (void)owner;
    }
}

void Projectile2DComponent::ProcessDeferredDestroys(GameWorld& world) noexcept {
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        Projectile2DComponent* projectile = object->GetComponent<Projectile2DComponent>();
        if (projectile == nullptr || !projectile->pendingDestroyOwner) {
            return;
        }
        projectile->pendingDestroyOwner = false;
        world.DestroyGameObject(object);
    });
}

void Projectile2DComponent::TickLifetime(const float deltaSeconds) noexcept {
    ageSeconds += deltaSeconds;
    if (lifetimeSeconds > 0.0F && ageSeconds >= lifetimeSeconds) {
        Deactivate();
    }
}

void Projectile2DComponent::MoveOwner(GameObject& owner, const float deltaSeconds) noexcept {
    TransformComponent* tr = owner.GetComponent<TransformComponent>();
    if (tr == nullptr) {
        return;
    }
    const Vector3 pos = tr->GetLocalTransform().translation;
    tr->SetTranslation(
            {pos.x + velocity.x * deltaSeconds, pos.y + velocity.y * deltaSeconds, pos.z});
}

void Projectile2DComponent::ResolveWorldPose(
        const GameObject& owner,
        float& centerX,
        float& centerY) const noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    const Vector3 worldPos = world.TranslationVector();
    centerX = worldPos.x;
    centerY = worldPos.y;
}

bool Projectile2DComponent::QuerySolidBlock(
        GameWorld& world,
        const float centerX,
        const float centerY) const noexcept {
    PhysicsQueryFilter2D solidFilter{};
    solidFilter.hitSolids = true;
    solidFilter.hitTriggers = false;

    Array<PhysicsQueryHit2D> hits{};
    if (shape == Projectile2DShape::Circle) {
        QueryOverlapCircleWorld2D(world, centerX, centerY, radius, solidFilter, hits);
    } else {
        const TransformComponent* tr = GetOwner() != nullptr ? GetOwner()->GetComponent<TransformComponent>() : nullptr;
        float scaleX = 1.0F;
        float scaleY = 1.0F;
        if (tr != nullptr) {
            const Vector3 scale = tr->GetLocalTransform().scale;
            scaleX = std::max(0.01F, std::fabs(scale.x));
            scaleY = std::max(0.01F, std::fabs(scale.y));
        }
        CollisionAabb2 aabb{};
        aabb.minX = centerX - halfExtents.x * scaleX;
        aabb.maxX = centerX + halfExtents.x * scaleX;
        aabb.minY = centerY - halfExtents.y * scaleY;
        aabb.maxY = centerY + halfExtents.y * scaleY;
        QueryOverlapAabbWorld2D(world, aabb, solidFilter, hits);
    }

    for (std::size_t i = 0; i < hits.GetSize(); ++i) {
        if (hits[i].owner == nullptr) {
            continue;
        }
        if (hits[i].owner == instigator || hits[i].owner == GetOwner()) {
            continue;
        }
        return true;
    }
    return false;
}

void Projectile2DComponent::QueryAndApplyHits(GameObject& owner) noexcept {
    float centerX = 0.0F;
    float centerY = 0.0F;
    ResolveWorldPose(owner, centerX, centerY);

    GameWorld& world = owner.GetWorld();
    Array<PhysicsQueryHit2D> staticHits{};
    Array<PhysicsQueryHitDynamic2D> dynamicHits{};

    if (shape == Projectile2DShape::Circle) {
        QueryOverlapCircleWorld2D(world, centerX, centerY, radius, targetFilter, staticHits);
        QueryOverlapCircleDynamics2D(
                world, centerX, centerY, radius, targetFilter, instigator, dynamicHits);
    } else {
        const TransformComponent* tr = owner.GetComponent<TransformComponent>();
        float scaleX = 1.0F;
        float scaleY = 1.0F;
        if (tr != nullptr) {
            const Vector3 scale = tr->GetLocalTransform().scale;
            scaleX = std::max(0.01F, std::fabs(scale.x));
            scaleY = std::max(0.01F, std::fabs(scale.y));
        }
        CollisionAabb2 aabb{};
        aabb.minX = centerX - halfExtents.x * scaleX;
        aabb.maxX = centerX + halfExtents.x * scaleX;
        aabb.minY = centerY - halfExtents.y * scaleY;
        aabb.maxY = centerY + halfExtents.y * scaleY;
        QueryOverlapAabbWorld2D(world, aabb, targetFilter, staticHits);
        QueryOverlapCircleDynamics2D(
                world,
                centerX,
                centerY,
                std::max(halfExtents.x * scaleX, halfExtents.y * scaleY),
                targetFilter,
                instigator,
                dynamicHits);
    }

    auto tryHit = [&](GameObject* victim) {
        if (victim == nullptr || victim == &owner || victim == instigator) {
            return false;
        }
        if (!victim->IsActiveInHierarchy()) {
            return false;
        }
        if (ArrayContainsObject(hitVictims, victim)) {
            return false;
        }
        Hurtbox2DComponent* hurtbox = victim->GetComponent<Hurtbox2DComponent>();
        const float applied = TryApplyCombatDamage2D(*victim, instigator, damage);
        if (applied <= 0.0F) {
            if (!deactivateOnFirstHit || hurtbox == nullptr || !hurtbox->IsEnabled()) {
                return false;
            }
            hitVictims.PushBack(victim);
            return true;
        }
        hitVictims.PushBack(victim);
        if (onHit) {
            onHit(owner, *victim, applied);
        }
        return true;
    };

    bool anyHit = false;
    for (std::size_t i = 0; i < staticHits.GetSize(); ++i) {
        if (tryHit(staticHits[i].owner)) {
            anyHit = true;
        }
    }
    for (std::size_t i = 0; i < dynamicHits.GetSize(); ++i) {
        if (tryHit(dynamicHits[i].owner)) {
            anyHit = true;
        }
    }

    if (anyHit && deactivateOnFirstHit) {
        MarkDeferredDestroy(owner);
    }
}

void Projectile2DComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (!active) {
        return;
    }

    MoveOwner(owner, timing.deltaTimeSeconds);

    float centerX = 0.0F;
    float centerY = 0.0F;
    ResolveWorldPose(owner, centerX, centerY);
    if (blockOnSolidHit && QuerySolidBlock(owner.GetWorld(), centerX, centerY)) {
        MarkDeferredDestroy(owner);
        return;
    }

    QueryAndApplyHits(owner);
    TickLifetime(timing.deltaTimeSeconds);
}

}  // namespace Spark
