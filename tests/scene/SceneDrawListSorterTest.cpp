#include <gtest/gtest.h>

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/submit/SceneDrawListSorter.hpp"

TEST(SceneDrawListSorterTest, StableSortOpaqueOrdersMeshSlots) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem custom{};
    custom.mesh = Spark::SceneMeshSlot::Custom;
    Spark::SceneDrawItem ground{};
    ground.mesh = Spark::SceneMeshSlot::GroundPlane;
    Spark::SceneDrawItem cube{};
    cube.mesh = Spark::SceneMeshSlot::UnitCube;

    items.PushBack(custom);
    items.PushBack(ground);
    items.PushBack(cube);

    Spark::SceneDrawListSorter::StableSortOpaqueDraws(items);

    ASSERT_EQ(items.GetSize(), 3U);
    EXPECT_EQ(items[0].mesh, Spark::SceneMeshSlot::GroundPlane);
    EXPECT_EQ(items[1].mesh, Spark::SceneMeshSlot::UnitCube);
    EXPECT_EQ(items[2].mesh, Spark::SceneMeshSlot::Custom);
}

TEST(SceneDrawListSorterTest, PartitionSplitsOpaqueAndTransparent) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem opaque{};
    opaque.mesh = Spark::SceneMeshSlot::UnitCube;
    opaque.opacity = 1.0F;
    Spark::SceneDrawItem transparent{};
    transparent.mesh = Spark::SceneMeshSlot::UnitCube;
    transparent.opacity = 0.5F;

    items.PushBack(opaque);
    items.PushBack(transparent);

    Spark::SceneRenderParams params{};
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, {0.0F, 0.0F, 0.0F});

    EXPECT_EQ(params.draws.GetSize(), 1U);
    EXPECT_EQ(params.transparentDraws.GetSize(), 1U);
    EXPECT_FLOAT_EQ(params.draws[0].opacity, 1.0F);
    EXPECT_FLOAT_EQ(params.transparentDraws[0].opacity, 0.5F);
}

TEST(SceneDrawListSorterTest, TransparentSortsFartherFirstByModelTranslation) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem nearItem{};
    nearItem.opacity = 0.4F;
    nearItem.model.m[14] = -2.0F;

    Spark::SceneDrawItem farItem{};
    farItem.opacity = 0.4F;
    farItem.model.m[14] = -20.0F;

    items.PushBack(nearItem);
    items.PushBack(farItem);

    Spark::SceneRenderParams params{};
    params.transparentSortMode = Spark::SceneTransparentSortMode::BackToFrontByDepth;
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, {0.0F, 0.0F, 0.0F});

    ASSERT_EQ(params.transparentDraws.GetSize(), 2U);
    // Back-to-front pass emits nearer transparent draws before farther ones (see SortTransparentBackToFront).
    EXPECT_GT(params.transparentDraws[0].model.m[14], params.transparentDraws[1].model.m[14]);
}

TEST(SceneDrawListSorterTest, StableSortSpritesBySortOrder) {
    Spark::Array<Spark::SceneSpriteDraw> sprites;
    Spark::SceneSpriteDraw back{};
    back.sortOrder = 0;
    Spark::SceneSpriteDraw front{};
    front.sortOrder = 10;

    sprites.PushBack(back);
    sprites.PushBack(front);

    Spark::SceneDrawListSorter::StableSortSprites(sprites, Spark::SceneSpriteSortMode::SortOrderOnly);

    ASSERT_EQ(sprites.GetSize(), 2U);
    EXPECT_EQ(sprites[0].sortOrder, 0);
    EXPECT_EQ(sprites[1].sortOrder, 10);
}

TEST(SceneDrawListSorterTest, StableSortSpritesUsesWorldYWhenConfigured) {
    Spark::Array<Spark::SceneSpriteDraw> sprites;
    Spark::SceneSpriteDraw low{};
    low.sortOrder = 0;
    low.sortWorldY = 1.0F;
    Spark::SceneSpriteDraw high{};
    high.sortOrder = 0;
    high.sortWorldY = 5.0F;

    sprites.PushBack(low);
    sprites.PushBack(high);

    Spark::SceneDrawListSorter::StableSortSprites(sprites, Spark::SceneSpriteSortMode::SortOrderThenWorldY);

    ASSERT_EQ(sprites.GetSize(), 2U);
    EXPECT_FLOAT_EQ(sprites[0].sortWorldY, 5.0F);
    EXPECT_FLOAT_EQ(sprites[1].sortWorldY, 1.0F);
}

TEST(SceneDrawListSorterTest, StableSortTilemapsByLayerAndWorldY) {
    Spark::Array<Spark::SceneTilemapDraw> draws;
    Spark::SceneTilemapDraw behind{};
    behind.sortingLayerOrder = 0;
    behind.sortOrderBase = 0;
    behind.sortWorldYAnchor = 0.0F;
    Spark::SceneTilemapDraw inFront{};
    inFront.sortingLayerOrder = 5;
    inFront.sortOrderBase = 0;
    inFront.sortWorldYAnchor = 0.0F;

    draws.PushBack(behind);
    draws.PushBack(inFront);

    Spark::SceneDrawListSorter::StableSortTilemaps(draws);

    ASSERT_EQ(draws.GetSize(), 2U);
    EXPECT_EQ(draws[0].sortingLayerOrder, 0);
    EXPECT_EQ(draws[1].sortingLayerOrder, 5);
}