#pragma once

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Gameplay frame hooks: post-physics flushes and shared interact dispatch.
 * Prefer <c>world.Simulation().RunGameplaySlice()</c> and <c>world.Gameplay().InteractPress</c>.
 */
class GameplaySubsystem final {
public:
    /** Flushes the world's pending destroy queue after 2D physics (idempotent if physics already flushed). */
    static void AfterPhysics2D(GameWorld& world) noexcept;

    /** Nearest valid interactable for an interact press. */
    [[nodiscard]] static bool ProcessInteractPress(GameWorld& world, GameObject& instigator) noexcept;
};

}  // namespace Spark
