#include "spark/ecs/components/gameplay/PickupComponent.hpp"

#include "spark/engine/IEngineContext.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace Spark {

void PickupComponent::OnSignal(GameObject& /*owner*/, const SignalId id, const SignalPayload& payload) {
    if (!autoCollectOnTriggerEnter || collected) {
        return;
    }
    if (id != SignalId::Physics2DTriggerEnter) {
        return;
    }
    GameObject* other = const_cast<GameObject*>(static_cast<const GameObject*>(payload.ptr));
    if (other == nullptr) {
        return;
    }
    if (TryCollect(*other) && destroyOwnerOnCollect) {
        pendingDestroyOwner = true;
    }
}

void PickupComponent::OnUpdate(
        const FrameTiming& /*timing*/,
        GameObject& owner,
        IEngineContext& /*context*/) {
    if (!pendingDestroyOwner) {
        return;
    }
    pendingDestroyOwner = false;
    owner.GetWorld().DestroyGameObject(&owner);
}

void PickupComponent::ProcessDeferredDestroys(GameWorld& world) noexcept {
    world.ForEachActiveGameObject([&](GameObject* object) {
        if (object == nullptr) {
            return;
        }
        PickupComponent* pickup = object->GetComponent<PickupComponent>();
        if (pickup == nullptr || !pickup->pendingDestroyOwner) {
            return;
        }
        pickup->pendingDestroyOwner = false;
        world.DestroyGameObject(object);
    });
}

bool PickupComponent::TryCollect(GameObject& collector) noexcept {
    if (collected) {
        return false;
    }
    collected = true;
    if (onCollected) {
        onCollected(collector, itemId.CStr(), quantity);
    }
    return true;
}

void PickupComponent::SetItemId(const char* id) noexcept {
    itemId = Utf8String(id != nullptr ? id : "item");
}

}  // namespace Spark
