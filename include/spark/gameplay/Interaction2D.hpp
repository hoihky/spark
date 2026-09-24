#pragma once

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Finds the nearest enabled <c>InteractableComponent</c> in range of <c>instigator</c> and
 * calls <c>TryInteract</c> when <c>interactPressed</c> is true.
 */
void ProcessInteractables2D(GameWorld& world, GameObject& instigator, bool interactPressed) noexcept;

}  // namespace Spark
