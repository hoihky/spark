#include "spark/scene/tilemap/TileGameplayProperties.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"
#include "spark/scene/tilemap/Tileset.hpp"

#include <gtest/gtest.h>

TEST(TileGameplayPropertiesTest, WalkableSetsForceWalkableAndNoCollision) {
    Spark::TileDefinition def{};
    Spark::TilemapPropertyList props{};
    Spark::TilemapObjectProperty walk{};
    walk.key = Spark::Utf8String("spark_walkable");
    walk.value = Spark::Utf8String("true");
    props.PushBack(walk);

    Spark::ApplyTileGameplayPropertiesToDefinition(def, props);
    EXPECT_EQ(def.collisionShape, Spark::TileCollisionShape::None);
    EXPECT_TRUE(def.HasFlag(Spark::TileDefinitionFlags::ForceWalkable));
}

TEST(TileGameplayPropertiesTest, CollisionPropertyMapsShapes) {
    Spark::TileDefinition def{};
    Spark::TilemapPropertyList props{};
    Spark::TilemapObjectProperty col{};
    col.key = Spark::Utf8String("spark_collision");
    col.value = Spark::Utf8String("bottom_half");
    props.PushBack(col);

    Spark::ApplyTileGameplayPropertiesToDefinition(def, props);
    EXPECT_EQ(def.collisionShape, Spark::TileCollisionShape::BottomHalf);
}
