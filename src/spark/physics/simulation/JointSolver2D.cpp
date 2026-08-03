#include "spark/physics/simulation/JointSolver2D.hpp"

#include "spark/physics/PhysicsJoints.hpp"
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/scene/GameWorld.hpp"

#include <algorithm>

namespace Spark {

void JointSolver2D::Solve(GameWorld& world, const PhysicsWorld2DSettings& settings) noexcept {
    const int jIters = std::max(0, settings.jointIterations);
    for (int j = 0; j < jIters; ++j) {
        const float scale = 1.0F / static_cast<float>(std::max(1, jIters));
        SolveDistanceJoints2D(world, scale);
        SolveHingeJoints2D(world, scale);
    }
}

}  // namespace Spark
