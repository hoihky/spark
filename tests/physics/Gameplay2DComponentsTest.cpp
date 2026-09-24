#include <gtest/gtest.h>

#include <algorithm>

#include "spark/ecs/components/animation/SpriteAnimatorComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/gameplay/InteractableComponent.hpp"
#include "spark/ecs/components/gameplay/PickupComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/CharacterController2DComponent.hpp"
#include "spark/ecs/components/physics/2d/OneWayPlatform2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/TriggerVolume2DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/gameplay/Interaction2D.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/scene/core/GameWorld.hpp"

TEST(Gameplay2DComponents, CharacterControllerJumpAndGround) {
    Spark::GameWorld world{};

    Spark::GameObject* platform = world.CreateGameObject();
    Spark::TransformComponent* platformTr = platform->AddComponent<Spark::TransformComponent>();
    platformTr->SetTranslation({0.0F, -1.0F, 0.0F});
    platformTr->SetScale({10.0F, 2.0F, 1.0F});
    platform->AddComponent<Spark::BoxCollider2DComponent>();

    Spark::GameObject* player = world.CreateGameObject();
    Spark::TransformComponent* playerTr = player->AddComponent<Spark::TransformComponent>();
    playerTr->SetTranslation({0.0F, 0.5F, 0.0F});
    player->AddComponent<Spark::BoxCollider2DComponent>();
    Spark::Rigidbody2DComponent* playerRb =
            player->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);
    auto* controller = player->AddComponent<Spark::CharacterController2DComponent>();
    controller->SetJumpSpeed(12.0F);

    Spark::PhysicsSubsystem physics{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;

    for (int i = 0; i < 90; ++i) {
        controller->SetMoveInputX(0.0F);
        physics.Simulate2D(world, timing);
    }
    EXPECT_TRUE(controller->IsGrounded());

    controller->RequestJump();
    physics.Simulate2D(world, timing);
    EXPECT_GT(playerRb->GetVelocity().y, 5.0F);
}

TEST(Gameplay2DComponents, TriggerVolumeEnterExitStay) {
    Spark::GameWorld world{};

    Spark::GameObject* zone = world.CreateGameObject();
    zone->AddComponent<Spark::TransformComponent>();
    auto* volume = zone->AddComponent<Spark::TriggerVolume2DComponent>(
            Spark::TriggerVolume2DShape::Box, Spark::Vector2{2.0F, 2.0F});
    int enterCount = 0;
    int stayCount = 0;
    int exitCount = 0;
    volume->SetOnEnter([&](Spark::GameObject&) { ++enterCount; });
    volume->SetOnStay([&](Spark::GameObject&) { ++stayCount; });
    volume->SetOnExit([&](Spark::GameObject&) { ++exitCount; });

    Spark::GameObject* probe = world.CreateGameObject();
    Spark::TransformComponent* probeTr = probe->AddComponent<Spark::TransformComponent>();
    probeTr->SetTranslation({0.0F, 0.0F, 0.0F});
    probe->AddComponent<Spark::BoxCollider2DComponent>();
    probe->AddComponent<Spark::Rigidbody2DComponent>();

    Spark::PhysicsSubsystem physics{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;

    physics.Simulate2D(world, timing);
    EXPECT_EQ(enterCount, 1);
    EXPECT_GE(stayCount, 0);

    physics.Simulate2D(world, timing);
    EXPECT_GE(stayCount, 1);

    probeTr->SetTranslation({10.0F, 0.0F, 0.0F});
    physics.Simulate2D(world, timing);
    EXPECT_EQ(exitCount, 1);
}

TEST(Gameplay2DComponents, PickupAutoCollectViaTriggerSignal) {
    Spark::GameWorld world{};

    Spark::GameObject* gem = world.CreateGameObject();
    gem->AddComponent<Spark::TransformComponent>();
    gem->AddComponent<Spark::TriggerVolume2DComponent>(Spark::TriggerVolume2DShape::Circle);
    auto* pickup = gem->AddComponent<Spark::PickupComponent>();
    pickup->SetItemId("gem");
    pickup->SetQuantity(1);
    int collected = 0;
    pickup->SetOnCollected([&](Spark::GameObject&, const char* id, int qty) {
        EXPECT_STREQ(id, "gem");
        EXPECT_EQ(qty, 1);
        ++collected;
    });

    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::TransformComponent>();
    player->AddComponent<Spark::BoxCollider2DComponent>();
    player->AddComponent<Spark::Rigidbody2DComponent>();

    Spark::PhysicsSubsystem physics{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;
    physics.Simulate2D(world, timing);

    EXPECT_EQ(collected, 1);
}

TEST(Gameplay2DComponents, InteractableManualCollect) {
    Spark::GameWorld world{};

    Spark::GameObject* chest = world.CreateGameObject();
    chest->AddComponent<Spark::TransformComponent>();
    auto* interactable = chest->AddComponent<Spark::InteractableComponent>();
    interactable->SetInteractionRadius(2.0F);
    auto* pickup = chest->AddComponent<Spark::PickupComponent>();
    pickup->SetAutoCollectOnTriggerEnter(false);
    pickup->SetDestroyOwnerOnCollect(false);
    bool collected = false;
    pickup->SetOnCollected([&](Spark::GameObject&, const char*, int) { collected = true; });
    interactable->SetOnInteract([&](Spark::GameObject& instigator) {
        chest->GetComponent<Spark::PickupComponent>()->TryCollect(instigator);
    });

    Spark::GameObject* player = world.CreateGameObject();
    Spark::TransformComponent* playerTr = player->AddComponent<Spark::TransformComponent>();
    playerTr->SetTranslation({0.5F, 0.0F, 0.0F});

    Spark::ProcessInteractables2D(world, *player, true);
    EXPECT_TRUE(collected);
}

TEST(Gameplay2DComponents, OneWayPlatformAllowsJumpThroughFromBelow) {
    Spark::GameWorld world{};

    Spark::GameObject* floor = world.CreateGameObject();
    Spark::TransformComponent* floorTr = floor->AddComponent<Spark::TransformComponent>();
    floorTr->SetTranslation({0.0F, -1.0F, 0.0F});
    floorTr->SetScale({12.0F, 2.0F, 1.0F});
    floor->AddComponent<Spark::BoxCollider2DComponent>();

    Spark::GameObject* oneWay = world.CreateGameObject();
    Spark::TransformComponent* oneWayTr = oneWay->AddComponent<Spark::TransformComponent>();
    oneWayTr->SetTranslation({0.0F, 2.0F, 0.0F});
    oneWayTr->SetScale({6.0F, 0.4F, 1.0F});
    oneWay->AddComponent<Spark::BoxCollider2DComponent>();
    oneWay->AddComponent<Spark::OneWayPlatform2DComponent>();

    Spark::GameObject* player = world.CreateGameObject();
    Spark::TransformComponent* playerTr = player->AddComponent<Spark::TransformComponent>();
    playerTr->SetTranslation({0.0F, 0.5F, 0.0F});
    player->AddComponent<Spark::BoxCollider2DComponent>();
    player->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Dynamic, 1.0F);
    auto* controller = player->AddComponent<Spark::CharacterController2DComponent>();
    controller->SetJumpSpeed(14.0F);

    Spark::PhysicsSubsystem physics{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;

    for (int i = 0; i < 60; ++i) {
        controller->SetMoveInputX(0.0F);
        physics.Simulate2D(world, timing);
    }
    EXPECT_TRUE(controller->IsGrounded());

    controller->RequestJump();
    float peakY = playerTr->GetLocalTransform().translation.y;
    for (int i = 0; i < 25; ++i) {
        controller->SetMoveInputX(0.0F);
        physics.Simulate2D(world, timing);
        peakY = std::max(peakY, playerTr->GetLocalTransform().translation.y);
    }

    EXPECT_GT(peakY, 2.0F);
}
