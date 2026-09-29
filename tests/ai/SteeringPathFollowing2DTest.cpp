#include "spark/ai/steering/SteeringBehaviors2D.hpp"

#include <gtest/gtest.h>

TEST(SteeringPathFollowing2DTest, AcceleratesTowardWaypoint) {
    Spark::Array<Spark::Vector2> path{};
    path.PushBack({10.0F, 0.0F});
    Spark::SteeringEnvironment2D env{};
    env.pathPoints = &path;
    env.maxSteeringSpeed = 4.0F;
    env.maxAcceleration = 8.0F;

    Spark::SteeringPathFollowing2D follow(1.0F);
    const Spark::Vector2 acc = follow.ComputeAcceleration({0.0F, 0.0F}, {0.0F, 0.0F}, env);
    EXPECT_GT(acc.x, 0.0F);
}
