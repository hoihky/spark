#include "spark/ecs/components/gameplay/CombatHurtFlash3DComponent.hpp"

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/gameplay/DamageTypes.hpp"

#include <algorithm>

namespace Spark {

void CombatHurtFlash3DComponent::SetFlashDuration(const float seconds) noexcept {
    flashDuration = std::max(0.01F, seconds);
}

void CombatHurtFlash3DComponent::CaptureBaseTintIfNeeded_(GameObject& owner) noexcept {
    if (hasBaseTint) {
        return;
    }
    if (MaterialComponent* material = owner.GetComponent<MaterialComponent>()) {
        baseTint = material->GetTint();
        hasBaseTint = true;
    }
}

void CombatHurtFlash3DComponent::ApplyFlashTint_(GameObject& owner) noexcept {
    MaterialComponent* material = owner.GetComponent<MaterialComponent>();
    if (material == nullptr) {
        return;
    }
    material->SetTint(hurtTint);
}

void CombatHurtFlash3DComponent::RestoreBaseTint_(GameObject& owner) noexcept {
    if (!hasBaseTint) {
        return;
    }
    if (MaterialComponent* material = owner.GetComponent<MaterialComponent>()) {
        material->SetTint(baseTint);
    }
}

void CombatHurtFlash3DComponent::OnSignal(
        GameObject& owner,
        const SignalId id,
        const SignalPayload& payload) {
    if (id != SignalId::DamageApplied) {
        return;
    }
    const DamageSignalPayload* damage = static_cast<const DamageSignalPayload*>(payload.ptr);
    if (damage == nullptr || damage->applied <= 0.0F) {
        return;
    }
    CaptureBaseTintIfNeeded_(owner);
    flashRemaining = flashDuration;
    ApplyFlashTint_(owner);
}

void CombatHurtFlash3DComponent::OnUpdate(
        const FrameTiming& timing,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (flashRemaining <= 0.0F) {
        return;
    }
    flashRemaining = std::max(0.0F, flashRemaining - timing.deltaTimeSeconds);
    if (flashRemaining <= 0.0F) {
        RestoreBaseTint_(owner);
    }
}

}  // namespace Spark
