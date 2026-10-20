#pragma once

namespace Spark {

class GameplayItemStack;

/** Game-specific inventory backend (UI bag, stats, quests). */
class IGameplayInventorySink {
public:
    virtual ~IGameplayInventorySink() = default;

    virtual bool TryReceive(const GameplayItemStack& stack) noexcept = 0;
};

}  // namespace Spark
