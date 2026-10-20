#pragma once

#include "spark/gameplay/GameplayInteractionQuery.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/** Dispatches interact input to the nearest interactable (2D or 3D distance). */
class GameplayInteractionProcessor final {
public:
    [[nodiscard]] bool ProcessInteractPress(GameWorld& world, GameObject& instigator) noexcept;

private:
    GameplayInteractionQuery query{};
};

}  // namespace Spark
