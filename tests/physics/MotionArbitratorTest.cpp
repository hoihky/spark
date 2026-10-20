#include <gtest/gtest.h>

#include "spark/character/KinematicCharacterMotor.hpp"
#include "spark/character/MotionArbitrationMode.hpp"
#include "spark/character/MotionArbitrator.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"

TEST(MotionArbitratorTest, GameplayDrivenIgnoresRootMotion) {
    Spark::GameWorld world{};
    Spark::GameObject* body = world.CreateGameObject();
    auto* transform = body->AddComponent<Spark::TransformComponent>();
    auto* controller = body->AddComponent<Spark::CharacterController3DComponent>();
    ASSERT_NE(transform, nullptr);

    Spark::KinematicCharacterMotor motor{};
    motor.Bind(controller, transform);

    Spark::MotionArbitrator arbitrator{};
    arbitrator.SetMode(Spark::MotionArbitrationMode::GameplayDriven);
    arbitrator.BeginFrame();
    arbitrator.SetGameplayWishVelocity({3.0F, 0.0F, 0.0F});
    arbitrator.AccumulateRootMotionDelta({1.0F, 0.0F, 0.0F});
    arbitrator.ApplyGameplayToMotor(motor);
    EXPECT_FLOAT_EQ(controller->GetMoveInput().x, 3.0F);
    arbitrator.ApplyRootMotionToMotor(motor);
    EXPECT_FLOAT_EQ(transform->GetLocalTransform().translation.x, 0.0F);
}

TEST(MotionArbitratorTest, RootMotionExclusiveAppliesDelta) {
    Spark::GameWorld world{};
    Spark::GameObject* body = world.CreateGameObject();
    auto* transform = body->AddComponent<Spark::TransformComponent>();
    auto* controller = body->AddComponent<Spark::CharacterController3DComponent>();
    ASSERT_NE(transform, nullptr);

    Spark::KinematicCharacterMotor motor{};
    motor.Bind(controller, transform);

    Spark::MotionArbitrator arbitrator{};
    arbitrator.SetMode(Spark::MotionArbitrationMode::RootMotionExclusive);
    arbitrator.BeginFrame();
    arbitrator.SetGameplayWishVelocity({3.0F, 0.0F, 0.0F});
    arbitrator.AccumulateRootMotionDelta({0.0F, 0.0F, 2.0F});
    arbitrator.ApplyGameplayToMotor(motor);
    EXPECT_FLOAT_EQ(controller->GetMoveInput().x, 0.0F);
    arbitrator.ApplyRootMotionToMotor(motor);
    EXPECT_FLOAT_EQ(transform->GetLocalTransform().translation.z, 2.0F);
}
