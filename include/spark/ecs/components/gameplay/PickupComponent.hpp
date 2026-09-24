#pragma once

#include "spark/core/Function.hpp"
#include "spark/core/Utf8String.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameObject;
class GameWorld;
class IEngineContext;

/**
 * Collectible item. Auto-collects on <c>Physics2DTriggerEnter</c> when enabled, or via
 * sibling <c>InteractableComponent::TryInteract</c>.
 */
class PickupComponent final : public GameComponent {
public:
    using CollectedCallback = Function<void(GameObject& collector, const char* itemId, int quantity)>;

    static constexpr ComponentKind TypeKind = ComponentKind::Pickup;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    void OnSignal(GameObject& owner, SignalId id, const SignalPayload& payload) override;
    void OnUpdate(const FrameTiming& timing, GameObject& owner, IEngineContext& context) override;

    /** Destroys owners that collected during the last trigger step (safe after <c>Simulate2D</c>). */
    static void ProcessDeferredDestroys(GameWorld& world) noexcept;

    void SetItemId(const char* id) noexcept;
    [[nodiscard]] const char* GetItemId() const noexcept { return itemId.CStr(); }

    void SetQuantity(const int value) noexcept { quantity = (value > 0) ? value : 1; }
    [[nodiscard]] int GetQuantity() const noexcept { return quantity; }

    void SetAutoCollectOnTriggerEnter(const bool value) noexcept { autoCollectOnTriggerEnter = value; }
    [[nodiscard]] bool GetAutoCollectOnTriggerEnter() const noexcept { return autoCollectOnTriggerEnter; }

    void SetDestroyOwnerOnCollect(const bool value) noexcept { destroyOwnerOnCollect = value; }
    [[nodiscard]] bool GetDestroyOwnerOnCollect() const noexcept { return destroyOwnerOnCollect; }

    void SetOnCollected(CollectedCallback callback) { onCollected = MoveTemp(callback); }

    [[nodiscard]] bool IsCollected() const noexcept { return collected; }

    /** Attempts collection once; returns true on first successful collect. */
    bool TryCollect(GameObject& collector) noexcept;

private:
    Utf8String itemId{"item"};
    int quantity = 1;
    bool autoCollectOnTriggerEnter = true;
    bool destroyOwnerOnCollect = true;
    bool collected = false;
    bool pendingDestroyOwner = false;
    CollectedCallback onCollected{};
};

}  // namespace Spark
