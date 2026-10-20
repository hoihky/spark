#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/gameplay/GameplayInventoryCollectorComponent.hpp"
#include "spark/ecs/components/gameplay/GameplayTeamComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/gameplay/InteractableComponent.hpp"
#include "spark/ecs/components/gameplay/PickupComponent.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/gameplay/Combat3D.hpp"
#include "spark/gameplay/DamageTypes.hpp"
#include "spark/gameplay/GameplayDamageEvent.hpp"
#include "spark/gameplay/GameplayInteractionProcessor.hpp"
#include "spark/gameplay/GameplayItemStack.hpp"
#include "spark/gameplay/GameplayPickupDelivery.hpp"
#include "spark/gameplay/GameplayTeamId.hpp"
#include "spark/scene/core/GameWorld.hpp"

TEST(GameplayKit, TeamPolicyBlocksFriendlyDamage) {
    Spark::GameWorld world{};
    Spark::GameObject* allyA = world.CreateGameObject();
    allyA->AddComponent<Spark::HealthComponent>(10.0F);
    allyA->AddComponent<Spark::GameplayTeamComponent>()->SetGameplayTeam(Spark::GameplayTeamId{1});

    Spark::GameObject* allyB = world.CreateGameObject();
    allyB->AddComponent<Spark::HealthComponent>(10.0F);
    allyB->AddComponent<Spark::GameplayTeamComponent>()->SetGameplayTeam(Spark::GameplayTeamId{1});

    const float applied = Spark::Combat3DTarget{*allyB}.ApplyDamage(allyA, 5.0F);
    EXPECT_FLOAT_EQ(applied, 0.0F);
    EXPECT_FLOAT_EQ(allyB->GetComponent<Spark::HealthComponent>()->GetCurrent(), 10.0F);
}

TEST(GameplayKit, TeamPolicyAllowsEnemyDamage) {
    Spark::GameWorld world{};
    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::GameplayTeamComponent>()->SetGameplayTeam(Spark::GameplayTeamId{1});

    Spark::GameObject* enemy = world.CreateGameObject();
    enemy->AddComponent<Spark::HealthComponent>(10.0F);
    enemy->AddComponent<Spark::GameplayTeamComponent>()->SetGameplayTeam(Spark::GameplayTeamId{2});

    const float applied = Spark::Combat3DTarget{*enemy}.ApplyDamage(player, 4.0F);
    EXPECT_GT(applied, 0.0F);
}

TEST(GameplayKit, DamageEventParsesDamageAppliedSignal) {
    Spark::GameWorld world{};
    Spark::GameObject* target = world.CreateGameObject();
    Spark::GameObject* instigator = world.CreateGameObject();

    Spark::DamageSignalPayload payload{};
    payload.amount = 7.0F;
    payload.applied = 5.0F;
    payload.instigator = instigator;
    payload.target = target;

    Spark::SignalPayload signal{};
    signal.ptr = &payload;

    Spark::GameplayDamageEvent event{};
    EXPECT_TRUE(Spark::GameplayDamageEvent::TryFromSignal(Spark::SignalId::DamageApplied, signal, event));
    EXPECT_FLOAT_EQ(event.GetRequestedAmount(), 7.0F);
    EXPECT_FLOAT_EQ(event.GetAppliedAmount(), 5.0F);
    EXPECT_EQ(event.GetInstigator(), instigator);
    EXPECT_EQ(event.GetTarget(), target);
}

TEST(GameplayKit, InteractionProcessorPicksNearestInteractable) {
    Spark::GameWorld world{};
    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 0.0F, 0.0F});

    Spark::GameObject* farChest = world.CreateGameObject();
    farChest->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{50.0F, 0.0F, 0.0F});
    auto* farInteract = farChest->AddComponent<Spark::InteractableComponent>();
    farInteract->SetInteractionRadius(2.0F);

    Spark::GameObject* nearChest = world.CreateGameObject();
    nearChest->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.5F, 0.0F, 0.0F});
    auto* nearInteract = nearChest->AddComponent<Spark::InteractableComponent>();
    nearInteract->SetInteractionRadius(2.0F);

    bool nearTriggered = false;
    nearInteract->SetOnInteract([&](Spark::GameObject& /*instigator*/) { nearTriggered = true; });

    Spark::GameplayInteractionProcessor processor{};
    EXPECT_TRUE(processor.ProcessInteractPress(world, *player));
    EXPECT_TRUE(nearTriggered);
}

TEST(GameplayKit, PickupDeliversToInventoryCollector) {
    Spark::GameWorld world{};
    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::GameplayInventoryCollectorComponent>();

    Spark::GameObject* gem = world.CreateGameObject();
    auto* pickup = gem->AddComponent<Spark::PickupComponent>();
    pickup->SetItemId("gem");
    pickup->SetQuantity(3);
    pickup->SetAutoCollectOnTriggerEnter(false);

    int collectedQty = 0;
    player->GetComponent<Spark::GameplayInventoryCollectorComponent>()->SetOnItemReceived(
            [&](Spark::GameObject& /*owner*/, const Spark::GameplayItemStack& stack) {
                collectedQty = stack.GetQuantity();
            });

    EXPECT_TRUE(pickup->TryCollect(*player));
    EXPECT_EQ(collectedQty, 3);
}

TEST(GameplayKit, PickupDeliveryFindsCollectorOnChild) {
    Spark::GameWorld world{};
    Spark::GameObject* player = world.CreateGameObject();
    Spark::GameObject* bag = world.CreateGameObject();
    bag->SetParent(player);
    bag->AddComponent<Spark::GameplayInventoryCollectorComponent>();

    EXPECT_TRUE(Spark::GameplayPickupDelivery::TryDeliverToCollector(*player, "coin", 2, nullptr));
}
