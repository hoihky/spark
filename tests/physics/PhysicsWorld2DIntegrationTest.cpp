#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/physics/PhysicsWorld2D.hpp"
#include "spark/scene/GameWorld.hpp"

TEST(PhysicsWorld2DIntegration, DynamicBoxRestsOnStaticPlatform) {
    Spark::GameWorld world{};

    Spark::GameObject* platform = world.CreateGameObject();
    Spark::TransformComponent* platformTr = platform->AddComponent<Spark::TransformComponent>();
    platformTr->SetTranslation({21.0F, -1.625F, 0.0F});
    platformTr->SetScale({66.0F, 3.25F, 1.0F});
    platform->AddComponent<Spark::BoxCollider2DComponent>();

    Spark::GameObject* player = world.CreateGameObject();
    Spark::TransformComponent* playerTr = player->AddComponent<Spark::TransformComponent>();
    playerTr->SetTranslation({-8.5F, 0.54F, 0.0F});
    playerTr->SetScale({0.8F, 1.08F, 1.0F});
    player->AddComponent<Spark::BoxCollider2DComponent>();
    Spark::Rigidbody2DComponent* playerRb =
            player->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);

    Spark::PhysicsWorld2D physics{};
    physics.GetSettings().gravityY = -32.0F;
    physics.GetSettings().maxFallSpeed = 46.0F;

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;

    for (int step = 0; step < 180; ++step) {
        physics.Simulate(world, timing);
    }

    const float playerY = playerTr->GetLocalTransform().translation.y;
    EXPECT_GT(playerY, -2.0F) << "player fell through the platform";
    EXPECT_LT(playerY, 2.0F) << "player was launched upward unexpectedly";
    EXPECT_NEAR(playerRb->GetVelocity().y, 0.0F, 0.5F);
    EXPECT_TRUE(playerRb->IsGrounded());
}

TEST(PhysicsWorld2DIntegration, DynamicBoxWithNegativeScaleRestsOnStaticPlatform) {
    Spark::GameWorld world{};

    Spark::GameObject* platform = world.CreateGameObject();
    Spark::TransformComponent* platformTr = platform->AddComponent<Spark::TransformComponent>();
    platformTr->SetTranslation({21.0F, -1.625F, 0.0F});
    platformTr->SetScale({66.0F, 3.25F, 1.0F});
    platform->AddComponent<Spark::BoxCollider2DComponent>();

    Spark::GameObject* player = world.CreateGameObject();
    Spark::TransformComponent* playerTr = player->AddComponent<Spark::TransformComponent>();
    playerTr->SetTranslation({-8.5F, 0.54F, 0.0F});
    playerTr->SetScale({-0.8F, 1.08F, 1.0F});
    player->AddComponent<Spark::BoxCollider2DComponent>();
    Spark::Rigidbody2DComponent* playerRb =
            player->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);

    Spark::PhysicsWorld2D physics{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;

    for (int step = 0; step < 180; ++step) {
        physics.Simulate(world, timing);
    }

    EXPECT_GT(playerTr->GetLocalTransform().translation.y, -2.0F);
    EXPECT_TRUE(playerRb->IsGrounded());
}

TEST(PhysicsWorld2DIntegration, LargeDeltaTimeDoesNotTunnelThroughFloor) {
    Spark::GameWorld world{};

    Spark::GameObject* platform = world.CreateGameObject();
    Spark::TransformComponent* platformTr = platform->AddComponent<Spark::TransformComponent>();
    platformTr->SetTranslation({21.0F, -1.625F, 0.0F});
    platformTr->SetScale({66.0F, 3.25F, 1.0F});
    platform->AddComponent<Spark::BoxCollider2DComponent>();

    Spark::GameObject* player = world.CreateGameObject();
    Spark::TransformComponent* playerTr = player->AddComponent<Spark::TransformComponent>();
    playerTr->SetTranslation({-8.5F, 0.54F, 0.0F});
    playerTr->SetScale({0.8F, 1.08F, 1.0F});
    player->AddComponent<Spark::BoxCollider2DComponent>();
    Spark::Rigidbody2DComponent* playerRb =
            player->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);

    Spark::PhysicsWorld2D physics{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.25F;

    physics.Simulate(world, timing);

    EXPECT_GT(playerTr->GetLocalTransform().translation.y, -2.0F);
    EXPECT_TRUE(playerRb->IsGrounded());
}
