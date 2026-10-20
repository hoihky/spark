#include "spark/gameplay/GameplayPickupDelivery.hpp"

#include "spark/ecs/components/gameplay/GameplayInventoryCollectorComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/gameplay/GameplayItemStack.hpp"

namespace Spark {

bool GameplayPickupDelivery::TryDeliverToCollector(
        GameObject& collector,
        const char* itemId,
        const int quantity,
        GameObject* pickupOwner) noexcept {
    GameplayInventoryCollectorComponent* sink = GameplayInventoryCollectorComponent::FindOnHierarchy(collector);
    if (sink == nullptr) {
        return false;
    }
    return sink->TryReceive(GameplayItemStack{itemId, quantity}, pickupOwner);
}

}  // namespace Spark
