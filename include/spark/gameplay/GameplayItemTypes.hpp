#pragma once

namespace Spark {

class GameObject;

/** Payload for <c>SignalId::ItemCollected</c> (valid only during synchronous dispatch). */
class GameplayItemCollectedPayload final {
public:
    const char* itemId = "item";
    int quantity = 1;
    GameObject* collector = nullptr;
    GameObject* pickupOwner = nullptr;
};

}  // namespace Spark
