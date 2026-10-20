#include "spark/ecs/components/physics/3d/Hurtbox3DComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/physics/core/ColliderFilter.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

void Hurtbox3DComponent::SetRadius(const float value) noexcept {
    radius = std::max(0.05F, value);
}

void Hurtbox3DComponent::SetInvulnerabilitySeconds(const float seconds) noexcept {
    invulnerabilitySeconds = std::max(0.0F, seconds);
}

void Hurtbox3DComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& /*owner*/,
        IEngineContext& /*context*/) {
    if (invulnerabilityRemaining > 0.0F) {
        invulnerabilityRemaining = std::max(0.0F, invulnerabilityRemaining - timing.deltaTimeSeconds);
    }
}

void Hurtbox3DComponent::NotifyDamageReceived() noexcept {
    if (invulnerabilitySeconds > 0.0F) {
        invulnerabilityRemaining = invulnerabilitySeconds;
    }
}

Vector3 Hurtbox3DComponent::GetWorldCenter(const GameObject& owner) const noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    return world.TranslationVector() + world.TransformVector(localOffset);
}

float Hurtbox3DComponent::GetWorldRadius(const GameObject& owner) const noexcept {
    const Matrix4 world = owner.GetWorldMatrix();
    const float sx = std::sqrt(
            world.m[0] * world.m[0] + world.m[1] * world.m[1] + world.m[2] * world.m[2]);
    const float sy = std::sqrt(
            world.m[4] * world.m[4] + world.m[5] * world.m[5] + world.m[6] * world.m[6]);
    const float sz = std::sqrt(
            world.m[8] * world.m[8] + world.m[9] * world.m[9] + world.m[10] * world.m[10]);
    const float scale = std::max({sx, sy, sz});
    return radius * scale;
}

bool Hurtbox3DComponent::MatchesQueryFilter(const ColliderFilter& queryFilter) const noexcept {
    ColliderFilter hurtboxFilter{};
    hurtboxFilter.categoryBits = categoryBits;
    hurtboxFilter.maskBits = maskBits;
    return hurtboxFilter.ShouldCollideWith(queryFilter);
}

bool Hurtbox3DComponent::OverlapsWorldSphere(
        const GameObject& owner,
        const Vector3& center,
        const float radius) const noexcept {
    const Vector3 hurtCenter = GetWorldCenter(owner);
    const float hurtRadius = GetWorldRadius(owner);
    const Vector3 delta{
            hurtCenter.x - center.x,
            hurtCenter.y - center.y,
            hurtCenter.z - center.z};
    const float reach = radius + hurtRadius;
    return delta.LengthSquared() <= reach * reach;
}

}  // namespace Spark
