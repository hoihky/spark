#pragma once

namespace Spark {

class GameWorld;

void SolveDistanceJoints2D(GameWorld& world, float stiffnessScale) noexcept;
void SolveHingeJoints2D(GameWorld& world, float stiffnessScale) noexcept;

void SolveHingeJoints3D(GameWorld& world, float stiffnessScale) noexcept;
void SolveSpringJoints3D(GameWorld& world, float deltaTimeSeconds) noexcept;

}  // namespace Spark
