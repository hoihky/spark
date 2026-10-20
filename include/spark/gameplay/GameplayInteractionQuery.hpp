#pragma once

namespace Spark {

class GameObject;
class GameWorld;
class GameplayInteractionCandidate;

/** Finds the nearest valid <c>InteractableComponent</c> for an instigator. */
class GameplayInteractionQuery final {
public:
    [[nodiscard]] GameplayInteractionCandidate FindBest(
            GameWorld& world,
            GameObject& instigator) const noexcept;
};

}  // namespace Spark
