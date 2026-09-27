#include "spark/ecs/components/physics/2d/Hurtbox2DComponent.hpp"

#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/GameObject.hpp"

#include <algorithm>

namespace Spark {

void Hurtbox2DComponent::SetRadius(const float value) noexcept {
    radius = std::max(0.05F, value);
    if (GameObject* owner = GetOwner()) {
        SyncCollider(*owner);
    }
}

void Hurtbox2DComponent::SetHalfExtents(const Vector2& value) noexcept {
    halfExtents.x = std::max(0.05F, value.x);
    halfExtents.y = std::max(0.05F, value.y);
    if (GameObject* owner = GetOwner()) {
        SyncCollider(*owner);
    }
}

void Hurtbox2DComponent::SetInvulnerabilitySeconds(const float seconds) noexcept {
    invulnerabilitySeconds = std::max(0.0F, seconds);
}

void Hurtbox2DComponent::SetCategoryBits(const std::uint16_t bits) noexcept {
    categoryBits = bits;
    if (GameObject* owner = GetOwner()) {
        SyncCollider(*owner);
    }
}

void Hurtbox2DComponent::SetMaskBits(const std::uint16_t bits) noexcept {
    maskBits = bits;
    if (GameObject* owner = GetOwner()) {
        SyncCollider(*owner);
    }
}

void Hurtbox2DComponent::OnAttach(GameObject& owner) {
    SyncCollider(owner);
}

void Hurtbox2DComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& /*owner*/,
        IEngineContext& /*context*/) {
    if (invulnerabilityRemaining > 0.0F) {
        invulnerabilityRemaining = std::max(0.0F, invulnerabilityRemaining - timing.deltaTimeSeconds);
    }
}

void Hurtbox2DComponent::NotifyDamageReceived() noexcept {
    if (invulnerabilitySeconds > 0.0F) {
        invulnerabilityRemaining = invulnerabilitySeconds;
    }
}

void Hurtbox2DComponent::SyncCollider(GameObject& owner) noexcept {
    if (!enabled) {
        return;
    }

    if (shape == Hurtbox2DShape::Circle) {
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

}  // namespace Spark
