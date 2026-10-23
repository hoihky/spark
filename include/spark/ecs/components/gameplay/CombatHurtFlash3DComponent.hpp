#pragma once

#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

/**
 * Brief material tint flash on <c>SignalId::DamageApplied</c> (for simple 3D props / training dummies).
 */
class CombatHurtFlash3DComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::CombatHurtFlash3D;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(set_flash_duration)
    void SetFlashDuration(const float seconds) noexcept;
    SPARK_SCRIPT_BIND(set_hurt_tint)
    void SetHurtTint(const Vector3& tint) noexcept { hurtTint = tint; }

private:
    void CaptureBaseTintIfNeeded_(GameObject& owner) noexcept;
    void ApplyFlashTint_(GameObject& owner) noexcept;
    void RestoreBaseTint_(GameObject& owner) noexcept;

    Vector3 hurtTint{1.0F, 0.35F, 0.35F};
    Vector3 baseTint{Vector3::One};
    float flashDuration = 0.14F;
    float flashRemaining = 0.0F;
    bool hasBaseTint = false;
};

}  // namespace Spark
