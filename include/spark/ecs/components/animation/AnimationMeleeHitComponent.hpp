#pragma once

#include "spark/core/Array.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/physics/3d/PhysicsQueries3D.hpp"

#include <cstdint>

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

    void SetStartEventName(const char* name) noexcept;
    void SetEndEventName(const char* name) noexcept;
    void SetFacingObject(GameObject* object) noexcept { facingObject = object; }
    void SetOriginLocalOffset(const Vector3& offset) noexcept { originLocalOffset = offset; }
    void SetTraceDistance(float meters) noexcept;
    void SetHitRadius(float meters) noexcept;
    void SetDamagePerHit(float amount) noexcept { damagePerHit = amount; }

    void SetWorldHurtboxQueryEnabled(const bool enabled) noexcept { worldHurtboxQueryEnabled = enabled; }
    [[nodiscard]] bool IsWorldHurtboxQueryEnabled() const noexcept { return worldHurtboxQueryEnabled; }

    void SetQueryFilter(const PhysicsQueryFilter3D& filter) noexcept { queryFilter = filter; }
    [[nodiscard]] const PhysicsQueryFilter3D& GetQueryFilter() const noexcept { return queryFilter; }

    void SetKnockbackMeters(const float meters) noexcept { knockbackMeters = meters > 0.0F ? meters : 0.0F; }
    void SetApplyHurtReaction(const bool enabled) noexcept { applyHurtReaction = enabled; }

    void ClearTargets() noexcept { targets.Clear(); }
    void AddTarget(GameObject* target) noexcept;

    [[nodiscard]] bool IsHitWindowActive() const noexcept { return hitWindowActive; }
    [[nodiscard]] std::uint32_t GetHitsThisSwing() const noexcept { return hitsThisSwing; }
    [[nodiscard]] std::uint32_t GetTotalHits() const noexcept { return totalHits; }

private:
    void ResetSwingHits() noexcept;
    void TryTraceHits(GameObject& owner) noexcept;
    void TryQueryWorldHurtboxes_(GameObject& owner, const Vector3& sampleCenter) noexcept;
    void RegisterHit_(GameObject& owner, GameObject& target) noexcept;
    [[nodiscard]] Vector3 ResolveForward_(const GameObject& owner) const noexcept;
    [[nodiscard]] Vector3 ResolveOrigin_(const GameObject& owner) const noexcept;

    GameObject* facingObject = nullptr;
    Array<GameObject*> targets{};
    Array<GameObject*> hitThisSwing{};
    Vector3 originLocalOffset{0.0F, 1.0F, 0.0F};
    float traceDistance = 2.2F;
    float hitRadius = 0.65F;
    float damagePerHit = 25.0F;
    float knockbackMeters = 0.35F;
    bool applyHurtReaction = true;
    bool worldHurtboxQueryEnabled = true;
    PhysicsQueryFilter3D queryFilter{};
    bool hitWindowActive = false;
    bool overlayHitWindow = false;
    std::uint32_t hitsThisSwing = 0;
    std::uint32_t totalHits = 0;
    Utf8String startEventName{"active_start"};
    Utf8String endEventName{"active_end"};
};

}  // namespace Spark
