#pragma once

#include "spark/math/Vector2.hpp"

namespace Spark {

class GameObject;
class GameWorld;

/**
 * Selects the best <c>CameraBounds2DComponent</c> volume containing the follow target and
 * writes min/max corners. Returns false when no zone applies (caller keeps rig defaults).
 */
[[nodiscard]] bool TryResolveCameraBounds2DForTarget(
        const GameWorld& world,
        const GameObject& followTarget,
        Vector2& boundsMinOut,
        Vector2& boundsMaxOut) noexcept;

}  // namespace Spark
