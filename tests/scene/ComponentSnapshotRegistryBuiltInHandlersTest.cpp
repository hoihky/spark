#include <gtest/gtest.h>

#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"

TEST(ComponentSnapshotRegistryBuiltInHandlersTest, CoreGameplayHandlersAreRegistered) {
    const Spark::ComponentSnapshotRegistry& registry = Spark::ComponentSnapshotRegistry::Default();
    EXPECT_NE(registry.Find(Spark::ComponentKind::Rigidbody2D), nullptr);
    EXPECT_NE(registry.Find(Spark::ComponentKind::SpawnPoint), nullptr);
    EXPECT_NE(registry.Find(Spark::ComponentKind::AiAgent), nullptr);
    EXPECT_NE(registry.Find(Spark::ComponentKind::PerceptionSensor), nullptr);
    EXPECT_NE(registry.Find(Spark::ComponentKind::TilemapMapSource), nullptr);
}

TEST(ComponentSnapshotRegistryBuiltInHandlersTest, GameFlowHandlersAreRegistered) {
    const Spark::ComponentSnapshotRegistry& registry = Spark::ComponentSnapshotRegistry::Default();
    EXPECT_NE(registry.Find(Spark::ComponentKind::GameState), nullptr);
    EXPECT_NE(registry.Find(Spark::ComponentKind::InputActionMap), nullptr);
    EXPECT_NE(registry.Find(Spark::ComponentKind::PlayerInput), nullptr);
}