#include <gtest/gtest.h>

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/sprites2d/Scene2DRuntimeLimits.hpp"
#include "spark/scene/submit/Scene2DSubmitCaps.hpp"

namespace {

using Spark::Scene2DRuntimeLimits;
using Spark::Scene2DSubmitCaps;
using Spark::SceneRenderParams;

}  // namespace

TEST(Scene2DSubmitCapsTest, LimitsMatchSceneRenderParams) {
    EXPECT_EQ(SceneRenderParams::MaxSprites, Scene2DRuntimeLimits::MaxSprites);
    EXPECT_EQ(SceneRenderParams::MaxSprites, Scene2DSubmitCaps::MaxSprites);
    EXPECT_EQ(SceneRenderParams::MaxTilemapDraws, Scene2DSubmitCaps::MaxTilemapDraws);
    EXPECT_EQ(SceneRenderParams::MaxTilemapTiles, Scene2DSubmitCaps::MaxTilemapTiles);
    EXPECT_EQ(SceneRenderParams::MaxParticles, Scene2DSubmitCaps::MaxParticles);
}

TEST(Scene2DSubmitCapsTest, RemainingTilemapTilesAccountsForUse) {
    SceneRenderParams params{};
    EXPECT_EQ(Scene2DSubmitCaps::RemainingTilemapTiles(params), Scene2DSubmitCaps::MaxTilemapTiles);
    for (std::uint32_t i = 0; i < 8U; ++i) {
        params.tilemapTiles.PushBack({});
    }
    EXPECT_EQ(Scene2DSubmitCaps::RemainingTilemapTiles(params), Scene2DSubmitCaps::MaxTilemapTiles - 8U);
}

TEST(Scene2DSubmitCapsTest, SpriteCapacityExhaustedWhenFull) {
    SceneRenderParams params{};
    EXPECT_TRUE(Scene2DSubmitCaps::HasSpriteCapacity(params));
    params.sprites.Resize(Scene2DSubmitCaps::MaxSprites);
    EXPECT_FALSE(Scene2DSubmitCaps::HasSpriteCapacity(params));
}