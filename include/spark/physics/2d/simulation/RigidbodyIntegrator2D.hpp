#pragma once

namespace Spark {

class GameWorld;
struct PhysicsWorld2DSettings;

/** Applies gravity, clamps fall speed, and integrates translation for dynamic 2D rigidbodies. */
class RigidbodyIntegrator2D {
public:
    static void Integrate(GameWorld& world, float deltaTimeSeconds, const PhysicsWorld2DSettings& settings) noexcept;
};

}  // namespace Spark
