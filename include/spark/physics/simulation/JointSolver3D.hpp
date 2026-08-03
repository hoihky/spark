#pragma once

namespace Spark {

class GameWorld;
struct PhysicsWorld3DSettings;

/** Distance, hinge, and spring joint iterations for one 3D physics substep. */
class JointSolver3D {
public:
    static void SolveSubstep(GameWorld& world, float substepDt, const PhysicsWorld3DSettings& settings) noexcept;
};

}  // namespace Spark
