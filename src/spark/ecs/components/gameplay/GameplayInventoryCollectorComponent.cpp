#include "spark/ecs/components/gameplay/GameplayInventoryCollectorComponent.hpp"

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/gameplay/GameplayItemStack.hpp"
#include "spark/gameplay/GameplayItemTypes.hpp"

namespace Spark {

bool GameplayInventoryCollectorComponent::TryReceive(const GameplayItemStack& stack) noexcept {
    return TryReceive(stack, nullptr);
}

bool GameplayInventoryCollectorComponent::TryReceive(
        const GameplayItemStack& stack,
        GameObject* pickupOwner) noexcept {
    GameObject* owner = GetOwner();
    if (owner == nullptr) {
        return false;
    }

    GameplayItemCollectedPayload payload{};
    payload.itemId = stack.GetItemId();
    payload.quantity = stack.GetQuantity();
    payload.collector = owner;
    payload.pickupOwner = pickupOwner;

    SignalPayload signal{};
    signal.ptr = &payload;
    owner->EmitSignal(SignalId::ItemCollected, signal, this);

    if (onItemReceived) {
        onItemReceived(*owner, stack);
    }
    return true;
}

GameplayInventoryCollectorComponent* GameplayInventoryCollectorComponent::FindOnHierarchy(
        GameObject& root) noexcept {
    if (GameplayInventoryCollectorComponent* local = root.GetComponent<GameplayInventoryCollectorComponent>()) {
        return local;
    }
    for (std::size_t i = 0; i < root.GetChildren().GetSize(); ++i) {
        if (GameObject* child = root.GetChildren()[i]) {
            if (GameplayInventoryCollectorComponent* found = FindOnHierarchy(*child)) {
                return found;
            }
        }
    }
    return nullptr;
}

}  // namespace Spark
