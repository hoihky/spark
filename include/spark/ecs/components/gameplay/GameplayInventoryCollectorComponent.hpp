#pragma once

#include "spark/core/Function.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/gameplay/IGameplayInventorySink.hpp"
#include "spark/scripting/SparkScriptBind.hpp"

namespace Spark {

class GameObject;
class GameplayItemStack;

/**
 * Default inventory hook on the player / collector object.
 * Implements <c>IGameplayInventorySink</c> and emits <c>SignalId::ItemCollected</c>.
 */
class GameplayInventoryCollectorComponent final : public GameComponent, public IGameplayInventorySink {
public:
    static constexpr ComponentKind TypeKind = ComponentKind::GameplayInventoryCollector;

    [[nodiscard]] ComponentKind Kind() const noexcept override { return TypeKind; }

    using ItemReceivedCallback = Function<void(GameObject& owner, const GameplayItemStack& stack)>;

    void SetOnItemReceived(ItemReceivedCallback callback) { onItemReceived = MoveTemp(callback); }

    bool TryReceive(const GameplayItemStack& stack) noexcept override;
    bool TryReceive(const GameplayItemStack& stack, GameObject* pickupOwner) noexcept;

    [[nodiscard]] static GameplayInventoryCollectorComponent* FindOnHierarchy(GameObject& root) noexcept;

private:
    ItemReceivedCallback onItemReceived{};
};

}  // namespace Spark
