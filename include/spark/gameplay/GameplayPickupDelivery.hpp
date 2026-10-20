#pragma once

namespace Spark {

class GameObject;

/** Routes collected pickups to a <c>GameplayInventoryCollectorComponent</c> when no custom callback is set. */
class GameplayPickupDelivery final {
public:
    [[nodiscard]] static bool TryDeliverToCollector(
            GameObject& collector,
            const char* itemId,
            int quantity,
            GameObject* pickupOwner = nullptr) noexcept;
};

}  // namespace Spark
