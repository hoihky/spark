
#include <gtest/gtest.h>

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapBrush.hpp"
#include "spark/scene/tilemap/TilemapEditSession.hpp"

namespace {

Spark::SharedPtr<Spark::Tileset> MakeTestTileset(Spark::GameWorld& world) {
    Spark::Texture2D placeholder{};
    placeholder.GetName() = Spark::Utf8String("tests/tilemap_edit_atlas");
    const Spark::SharedPtr<Spark::Texture2D> atlas = world.RegisterTexture(
            Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(placeholder)), "tests/tilemap_edit_atlas");
    return Spark::MakeShared<Spark::Tileset>(atlas, 4U, 4U);
}

}  // namespace

TEST(TilemapEditSession, PaintUndoRedoAndRevision) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeTestTileset(world), 4U, 4U, 1.0F, 0);

    Spark::TilemapEditSession session{};
    ASSERT_TRUE(session.Attach(*map));

    Spark::TilemapBrush brush{};
    brush.mode = Spark::TilemapBrush::Mode::Single;
    brush.single = Spark::TileCell::FromTileId(3U);
    session.SetBrush(brush);

    EXPECT_TRUE(session.PaintCell(1U, 1U));
    EXPECT_EQ(tilemap->GetTileCell(0U, 1U, 1U).tileId, 3U);
    EXPECT_EQ(session.GetRevisionTracker().GetCurrentRevision(), 1U);
    EXPECT_FALSE(session.CanRedo());

    EXPECT_TRUE(session.Undo());
    EXPECT_TRUE(tilemap->GetTileCell(0U, 1U, 1U).IsEmpty());
    EXPECT_TRUE(session.CanRedo());

    EXPECT_TRUE(session.Redo());
    EXPECT_EQ(tilemap->GetTileCell(0U, 1U, 1U).tileId, 3U);
}

TEST(TilemapEditSession, GestureCoalescesStroke) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeTestTileset(world), 6U, 1U, 1.0F, 0);

    Spark::TilemapEditSession session{};
    ASSERT_TRUE(session.Attach(*map));

    Spark::TilemapBrush brush{};
    brush.mode = Spark::TilemapBrush::Mode::Single;
    brush.single = Spark::TileCell::FromTileId(2U);
    session.SetBrush(brush);

    session.BeginGesture();
    EXPECT_TRUE(session.PaintCell(0U, 0U));
    EXPECT_TRUE(session.PaintCell(1U, 0U));
    EXPECT_TRUE(session.PaintCell(2U, 0U));
    session.EndGesture();

    EXPECT_EQ(session.GetRevisionTracker().GetCurrentRevision(), 1U);
    EXPECT_TRUE(session.Undo());
    EXPECT_TRUE(tilemap->GetTileCell(0U, 0U, 0U).IsEmpty());
    EXPECT_TRUE(tilemap->GetTileCell(0U, 2U, 0U).IsEmpty());
}

TEST(TilemapEditSession, LineRectFillAndResize) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeTestTileset(world), 5U, 5U, 1.0F, 0);

    Spark::TilemapEditSession session{};
    ASSERT_TRUE(session.Attach(*map));

    Spark::TilemapBrush brush{};
    brush.mode = Spark::TilemapBrush::Mode::Single;
    brush.single = Spark::TileCell::FromTileId(4U);
    session.SetBrush(brush);

    EXPECT_TRUE(session.PaintLine(0U, 0U, 4U, 0U));
    EXPECT_EQ(tilemap->GetTileCell(0U, 2U, 0U).tileId, 4U);

    EXPECT_TRUE(session.PaintRect(1U, 1U, 3U, 3U, true));
    EXPECT_EQ(tilemap->GetTileCell(0U, 2U, 2U).tileId, 4U);

    tilemap->SetTileCell(0U, 0U, 4U, Spark::TileCell::FromTileId(1U));
    tilemap->SetTileCell(0U, 1U, 4U, Spark::TileCell::FromTileId(1U));
    session.GetDocumentMutable().tileLayers[0U].cells[0U + 4U * 5U] = Spark::TileCell::FromTileId(1U);
    session.GetDocumentMutable().tileLayers[0U].cells[1U + 4U * 5U] = Spark::TileCell::FromTileId(1U);
    EXPECT_TRUE(session.FloodFill(0U, 4U));
    EXPECT_EQ(tilemap->GetTileCell(0U, 1U, 4U).tileId, 4U);

    EXPECT_TRUE(session.ResizeMap(6U, 6U));
    EXPECT_EQ(tilemap->GetMapWidth(), 6U);
    EXPECT_EQ(tilemap->GetTileCell(0U, 2U, 0U).tileId, 4U);
    EXPECT_TRUE(session.Undo());
    EXPECT_EQ(tilemap->GetMapWidth(), 5U);
}

TEST(TilemapEditSession, TerrainAndRandomBrushes) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    map->AddComponent<Spark::TilemapComponent>(MakeTestTileset(world), 2U, 2U, 1.0F, 0);

    Spark::TilemapEditSession session{};
    ASSERT_TRUE(session.Attach(*map));

    Spark::TilemapBrush terrain{};
    terrain.mode = Spark::TilemapBrush::Mode::TerrainPaint;
    terrain.terrainPaintId = 5U;
    session.SetBrush(terrain);
    EXPECT_TRUE(session.PaintCell(0U, 0U));
    EXPECT_EQ(session.GetDocument().tileLayers[0U].cells[0U].paintTileId, 5U);

    Spark::TilemapBrush random{};
    random.mode = Spark::TilemapBrush::Mode::RandomPalette;
    random.paletteTileIds.PushBack(7U);
    random.paletteTileIds.PushBack(8U);
    random.randomSeed = 42U;
    session.SetBrush(random);
    EXPECT_TRUE(session.PaintCell(1U, 1U));
    const std::uint16_t id = session.GetDocument().tileLayers[0U].cells[1U + 1U * 2U].tileId;
    EXPECT_TRUE(id == 7U || id == 8U);
}
