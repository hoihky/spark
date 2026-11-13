#pragma once

namespace Spark {

class GameWorld;
struct PhysicsWorld2DSettings;

/** Soft position iterations for 2D distance and hinge joints. */
class JointSolver2D {
public:
    static void Solve(GameWorld& world, const PhysicsWorld2DSettings& settings) noexcept;
};

}  // namespace Spark
