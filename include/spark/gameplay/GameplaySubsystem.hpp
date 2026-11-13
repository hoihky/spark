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
    /** Pickup deferred destroys (call after 2D physics; projectiles flush inside <c>PhysicsSubsystem::Simulate2D</c>). */
    static void AfterPhysics2D(GameWorld& world) noexcept;

    /** Nearest valid interactable for an interact press. */
    [[nodiscard]] static bool ProcessInteractPress(GameWorld& world, GameObject& instigator) noexcept;
};

}  // namespace Spark
