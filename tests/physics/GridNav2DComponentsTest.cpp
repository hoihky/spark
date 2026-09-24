#include <gtest/gtest.h>

#include "spark/ai/NavigationSubsystem.hpp"
#include "spark/ecs/components/ai/GridNavAgent2DComponent.hpp"
#include "spark/ecs/components/ai/GridNavTarget2DComponent.hpp"
#include "spark/ecs/components/ai/GridPathFollower2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"

namespace {

Spark::SharedPtr<Spark::Tileset> MakeTestTileset(Spark::GameWorld& world) {
    Spark::Texture2D placeholder{};
    placeholder.GetName() = Spark::Utf8String("tests/grid_nav_atlas");
    const Spark::SharedPtr<Spark::Texture2D> atlas =
            world.RegisterTexture(Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(placeholder)), "tests/grid_nav_atlas");
    return Spark::MakeShared<Spark::Tileset>(atlas, 8U, 8U);
}

}  // namespace

TEST(GridNav2DComponents, AgentFindsPathAcrossOpenGrid) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeTestTileset(world), 6U, 6U, 1.0F, 0);
    for (std::uint32_t y = 0; y < 6U; ++y) {
        for (std::uint32_t x = 0; x < 6U; ++x) {
            tilemap->SetTile(0U, x, y, 1U);
        }
    }
    tilemap->SetTile(0U, 2U, 2U, 0U);
    auto* grid = map->AddComponent<Spark::TilemapGameplayGridComponent>();
    grid->SetWalkRule(Spark::TilemapGameplayWalkRule::OccupiedWalkable);
    grid->RebakeIfNeeded(*map);

    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::TransformComponent>()->SetTranslation({0.5F, 0.5F, 0.0F});
    auto* nav = player->AddComponent<Spark::GridNavAgent2DComponent>();
    nav->SetGridSourceObject(map);
    nav->SetGoalMode(Spark::GridNavGoalMode2D::GridCell);
    Spark::GridPathfinder::Cell goal{};
    goal.x = 5;
    goal.y = 5;
    nav->SetGoalCell(goal);
    nav->SetRepathEveryFrame(true);

    Spark::ProcessGridNavAgents2D(world, 0.016F);
    EXPECT_TRUE(nav->HasPath());
    EXPECT_GT(nav->GetWorldWaypoints().GetSize(), 1U);
}

TEST(GridNav2DComponents, PathFollowerLinksToAgent) {
    Spark::GameWorld world{};
    Spark::GameObject* owner = world.CreateGameObject();
    auto* nav = owner->AddComponent<Spark::GridNavAgent2DComponent>();
    auto* follower = owner->AddComponent<Spark::GridPathFollower2DComponent>();
    follower->SetNavAgent(nav);
    follower->SetMaxSpeed(9.0F);
    EXPECT_EQ(follower->GetNavAgent(), nav);
    EXPECT_FLOAT_EQ(follower->GetMaxSpeed(), 9.0F);
    EXPECT_EQ(follower->UpdatePriority(), 125);
}

TEST(GridNav2DComponents, TargetObjectResolvesGoalCell) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeTestTileset(world), 4U, 4U, 1.0F, 0);
    tilemap->SetTile(0U, 1U, 1U, 1U);
    auto* grid = map->AddComponent<Spark::TilemapGameplayGridComponent>();
    grid->SetWalkRule(Spark::TilemapGameplayWalkRule::OccupiedWalkable);
    grid->RebakeIfNeeded(*map);

    Spark::GameObject* target = world.CreateGameObject();
    target->AddComponent<Spark::TransformComponent>()->SetTranslation({2.5F, 2.5F, 0.0F});
    target->AddComponent<Spark::GridNavTarget2DComponent>();

    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::TransformComponent>()->SetTranslation({0.5F, 0.5F, 0.0F});
    auto* nav = player->AddComponent<Spark::GridNavAgent2DComponent>();
    nav->SetGridSourceObject(map);
    nav->SetGoalMode(Spark::GridNavGoalMode2D::TargetObject);
    nav->SetGoalTarget(target);

    Spark::ProcessGridNavAgents2D(world, 0.016F);
    EXPECT_TRUE(nav->HasPath());
}
