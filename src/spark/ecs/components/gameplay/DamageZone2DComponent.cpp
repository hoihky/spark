#include "spark/ecs/components/gameplay/DamageZone2DComponent.hpp"

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
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

void ResolveWorldCenter(const GameObject& owner, const Vector2& localOffset, float& centerX, float& centerY) noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    const Vector3 worldPos = world.TranslationVector();
    centerX = worldPos.x + localOffset.x;
    centerY = worldPos.y + localOffset.y;
}

}  // namespace

void DamageZone2DComponent::SetRadius(const float value) noexcept {
    radius = std::max(0.05F, value);
    if (GameObject* owner = GetOwner()) {
        SyncCollider(*owner);
    }
}

void DamageZone2DComponent::SetHalfExtents(const Vector2& value) noexcept {
    halfExtents.x = std::max(0.05F, value.x);
    halfExtents.y = std::max(0.05F, value.y);
    if (GameObject* owner = GetOwner()) {
        SyncCollider(*owner);
    }
}

void DamageZone2DComponent::SetCategoryBits(const std::uint16_t bits) noexcept {
    categoryBits = bits;
    if (GameObject* owner = GetOwner()) {
        SyncCollider(*owner);
    }
}

void DamageZone2DComponent::SetMaskBits(const std::uint16_t bits) noexcept {
    maskBits = bits;
    if (GameObject* owner = GetOwner()) {
        SyncCollider(*owner);
    }
}

void DamageZone2DComponent::OnAttach(GameObject& owner) {
    SyncCollider(owner);
}

void DamageZone2DComponent::SyncCollider(GameObject& owner) noexcept {
    if (!enabled) {
        return;
    }

    if (shape == DamageZone2DShape::Circle) {
        CircleCollider2DComponent* circle = owner.GetComponent<CircleCollider2DComponent>();
        if (circle == nullptr) {
            circle = owner.AddComponent<CircleCollider2DComponent>(radius, localOffset);
        } else {
            circle->SetRadius(radius);
            circle->SetOffset(localOffset);
        }
        circle->SetIsTrigger(true);
        circle->SetCategoryBits(categoryBits);
        circle->SetMaskBits(maskBits);
        return;
    }

    BoxCollider2DComponent* box = owner.GetComponent<BoxCollider2DComponent>();
    if (box == nullptr) {
        box = owner.AddComponent<BoxCollider2DComponent>(halfExtents, localOffset);
    } else {
        box->SetHalfExtents(halfExtents);
        box->SetOffset(localOffset);
    }
    box->SetIsTrigger(true);
    box->SetCategoryBits(categoryBits);
    box->SetMaskBits(maskBits);
}

void DamageZone2DComponent::ApplyDamageInVolume(GameObject& owner, const float deltaSeconds) noexcept {
    if (damagePerSecond <= 0.0F || deltaSeconds <= 0.0F) {
        return;
    }

    float centerX = 0.0F;
    float centerY = 0.0F;
    ResolveWorldCenter(owner, localOffset, centerX, centerY);

    GameWorld& world = owner.GetWorld();
    const float tickDamage = damagePerSecond * deltaSeconds;
    GameObject* damageInstigator = instigator != nullptr ? instigator : &owner;

    Array<PhysicsQueryHit2D> staticHits{};
    Array<PhysicsQueryHitDynamic2D> dynamicHits{};

    if (shape == DamageZone2DShape::Circle) {
        QueryOverlapCircleWorld2D(world, centerX, centerY, radius, targetFilter, staticHits);
        QueryOverlapCircleDynamics2D(world, centerX, centerY, radius, targetFilter, damageInstigator, dynamicHits);
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
                damageInstigator,
                dynamicHits);
    }

    Array<GameObject*> victims{};
    auto collectVictim = [&](GameObject* victim) {
        if (victim == nullptr || victim == &owner || victim == damageInstigator) {
            return;
        }
        if (!victim->IsActiveInHierarchy()) {
            return;
        }
        if (ArrayContainsObject(victims, victim)) {
            return;
        }
        victims.PushBack(victim);
    };

    for (std::size_t i = 0; i < staticHits.GetSize(); ++i) {
        collectVictim(staticHits[i].owner);
    }
    for (std::size_t i = 0; i < dynamicHits.GetSize(); ++i) {
        collectVictim(dynamicHits[i].owner);
    }

    for (std::size_t i = 0; i < victims.GetSize(); ++i) {
        TryApplyCombatDamage2D(*victims[i], damageInstigator, tickDamage);
    }
}

void DamageZone2DComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (!enabled) {
        return;
    }
    ApplyDamageInVolume(owner, timing.deltaTimeSeconds);
}

}  // namespace Spark
