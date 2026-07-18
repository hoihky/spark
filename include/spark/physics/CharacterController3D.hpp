#pragma once

#include "spark/engine/FrameTiming.hpp"

namespace Spark {

class GameWorld;

struct CharacterController3DSettings {
    float gravityY = -24.0F;
    float maxFallSpeed = 80.0F;
    /** Slide-and-sweep iterations per substep when resolving translation vs static boxes. */
    int slideIterations = 3;
    /** Binary-search steps when shortening a translation segment before overlap. */
    int sweepBinaryIterations = 8;
    float broadPhaseCellSize = 2.0F;
};

/**
 * Integrates every active <c>CharacterController3DComponent</c> against static <c>BoxCollider3DComponent</c> and
 * <c>CapsuleCollider3DComponent</c>
 * geometry (same broad-phase as <c>SimulatePhysics3D</c>). Objects with a character controller are excluded from
 * dynamic sphere rigidbody simulation.
 */
void SimulateCharacterControllers3D(
        GameWorld& world,
        const FrameTiming& timing,
        const CharacterController3DSettings& settings = {});

}  // namespace Spark
