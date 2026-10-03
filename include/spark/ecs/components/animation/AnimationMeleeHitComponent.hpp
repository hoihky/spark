#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"

#include <cstdint>
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;

/**
 * Enables a forward melee hit trace while an animation event window is open
 * (<c>active_start</c> … <c>active_end</c> by default). Listens for
 * <c>SignalId::AnimationEvent</c> from a sibling <c>AnimationEventReceiverComponent</c>.
 */
class AnimationMeleeHitComponent final : public GameComponent {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::AnimationMeleeHit;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    [[nodiscard]] int UpdatePriority() const noexcept override { return 220; }

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    SPARK_SCRIPT_BIND(set_start_event_name)
    void SetStartEventName(const char* name) noexcept;
    SPARK_SCRIPT_BIND(set_end_event_name)
    void SetEndEventName(const char* name) noexcept;
    SPARK_SCRIPT_BIND(set_facing_object)
    void SetFacingObject(GameObject* object) noexcept { facingObject = object; }
    SPARK_SCRIPT_BIND(set_origin_local_offset)
    void SetOriginLocalOffset(const Vector3& offset) noexcept { originLocalOffset = offset; }
    SPARK_SCRIPT_BIND(set_trace_distance)
    void SetTraceDistance(float meters) noexcept;
    SPARK_SCRIPT_BIND(set_hit_radius)
    void SetHitRadius(float meters) noexcept;
    SPARK_SCRIPT_BIND(set_damage_per_hit)
    void SetDamagePerHit(float amount) noexcept { damagePerHit = amount; }

    SPARK_SCRIPT_BIND(clear_targets)
    void ClearTargets() noexcept { targets.Clear(); }
    SPARK_SCRIPT_BIND(add_target)
    void AddTarget(GameObject* target) noexcept;

    SPARK_SCRIPT_BIND(is_hit_window_active)
    [[nodiscard]] bool IsHitWindowActive() const noexcept { return hitWindowActive; }
    SPARK_SCRIPT_BIND(get_hits_this_swing)
    [[nodiscard]] std::uint32_t GetHitsThisSwing() const noexcept { return hitsThisSwing; }
    SPARK_SCRIPT_BIND(get_total_hits)
    [[nodiscard]] std::uint32_t GetTotalHits() const noexcept { return totalHits; }

private:
    void ResetSwingHits() noexcept;
    void TryTraceHits(GameObject& owner) noexcept;
    [[nodiscard]] Vector3 ResolveForward_(const GameObject& owner) const noexcept;
    [[nodiscard]] Vector3 ResolveOrigin_(const GameObject& owner) const noexcept;

    GameObject* facingObject = nullptr;
    Array<GameObject*> targets{};
    Array<GameObject*> hitThisSwing{};
    Vector3 originLocalOffset{0.0F, 1.0F, 0.0F};
    float traceDistance = 2.2F;
    float hitRadius = 0.65F;
    float damagePerHit = 25.0F;
    bool hitWindowActive = false;
    std::uint32_t hitsThisSwing = 0;
    std::uint32_t totalHits = 0;
    Utf8String startEventName{"active_start"};
    Utf8String endEventName{"active_end"};
};

}  // namespace Spark
