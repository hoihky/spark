#include <gtest/gtest.h>

#include "spark/core/Array.hpp"
#include "spark/scene/lod/MeshLodLevel.hpp"
#include "spark/scene/lod/MeshLodSelector.hpp"

TEST(MeshLodSelectorTest, HysteresisDelaysCoarserLod) {
    Spark::Array<Spark::MeshLodLevel> levels{};
    levels.PushBack(Spark::MeshLodLevel{.minDistance = 0.0F});
    levels.PushBack(Spark::MeshLodLevel{.minDistance = 20.0F});
    std::uint32_t active = 0U;

    EXPECT_EQ(Spark::MeshLodSelector::SelectIndex(levels, 10.0F, active, 2.0F), 0U);
    EXPECT_EQ(Spark::MeshLodSelector::SelectIndex(levels, 21.0F, active, 2.0F), 0U);
    EXPECT_EQ(Spark::MeshLodSelector::SelectIndex(levels, 22.0F, active, 2.0F), 1U);
    EXPECT_EQ(Spark::MeshLodSelector::SelectIndex(levels, 21.5F, active, 2.0F), 1U);
    EXPECT_EQ(Spark::MeshLodSelector::SelectIndex(levels, 19.0F, active, 2.0F), 1U);
    EXPECT_EQ(Spark::MeshLodSelector::SelectIndex(levels, 17.9F, active, 2.0F), 0U);
}