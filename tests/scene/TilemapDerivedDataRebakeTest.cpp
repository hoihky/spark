#include <gtest/gtest.h>

#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapAutotileComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/tilemap/TileAutotile.hpp"
#include "spark/scene/tilemap/TileDefinition.hpp"
#include "spark/scene/tilemap/TilemapBrush.hpp"
#include "spark/scene/tilemap/TilemapDerivedDataRebake.hpp"
#include "spark/scene/tilemap/TilemapEditRevision.hpp"
#include "spark/scene/tilemap/TilemapEditSession.hpp"
#include "spark/scene/tilemap/TilemapGameplayWalkRule.hpp"

namespace {

Spark::SharedPtr<Spark::Tileset> MakeWalkableTileset(Spark::GameWorld& world) {
    Spark::Texture2D placeholder{};
    placeholder.GetName() = Spark::Utf8String("tests/derived_rebake_atlas");
    const Spark::SharedPtr<Spark::Texture2D> atlas = world.RegisterTexture(
            Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(placeholder)), "tests/derived_rebake_atlas");
    Spark::SharedPtr<Spark::Tileset> tileset = Spark::MakeShared<Spark::Tileset>(atlas, 4U, 4U);
    tileset->EnsureDefinitions();
    tileset->Definition(1U).flags = Spark::TileDefinitionFlags::None;
    tileset->Definition(2U).flags = Spark::TileDefinitionFlags::BlocksPathfinding;
    return tileset;
}

Spark::SharedPtr<Spark::Tileset> MakeAutotileTileset(Spark::GameWorld& world) {
    Spark::SharedPtr<Spark::Tileset> tileset = MakeWalkableTileset(world);
    tileset->Definition(10U).autotileGroup = 1U;
    Spark::TileAutotileRuleSet& rules = tileset->GetOrCreateAutotileRuleSet(1U);
    rules.variants[0U].tileId = 20U;
    rules.variants[2U].tileId = 21U;
    return tileset;
}

}  // namespace

TEST(TilemapDerivedDataRebake, RegionFromRevisionExpandsMargin) {
    Spark::TilemapEditRevision revision{};
    revision.kind = Spark::TilemapEditChangeKind::CellPaint;
    revision.dirtyRegion = Spark::TilemapCellRegion::SingleCell(0U, 2U, 2U);

    const Spark::TilemapCellRegion expanded = Spark::TilemapCellRegion::FromRevision(revision, 8U, 8U, 1U);
    EXPECT_EQ(expanded.minX, 1U);
    EXPECT_EQ(expanded.maxX, 3U);
    EXPECT_EQ(expanded.minY, 1U);
    EXPECT_EQ(expanded.maxY, 3U);
}

TEST(TilemapDerivedDataRebake, PartialGameplayGridRebake) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeWalkableTileset(world), 4U, 4U, 1.0F, 0);
    for (std::uint32_t y = 0; y < 4U; ++y) {
        for (std::uint32_t x = 0; x < 4U; ++x) {
            tilemap->SetTile(0U, x, y, 1U);
        }
    }
    auto* grid = map->AddComponent<Spark::TilemapGameplayGridComponent>();
    grid->SetWalkRule(Spark::TilemapGameplayWalkRule::OccupiedWalkable);
    grid->RebakeIfNeeded(*map);
    EXPECT_TRUE(grid->GetWalkability().IsWalkable(0, 0));

    tilemap->SetTile(0U, 3U, 3U, 2U);
    const Spark::TilemapCellRegion region = Spark::TilemapCellRegion::SingleCell(0U, 3U, 3U);
    grid->RebakeRegion(*map, region);
    EXPECT_FALSE(grid->GetWalkability().IsWalkable(3, 3));
    EXPECT_TRUE(grid->GetWalkability().IsWalkable(0, 0));
}

TEST(TilemapDerivedDataRebake, EditSessionWiresRevisionToRebake) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeWalkableTileset(world), 3U, 3U, 1.0F, 0);
    tilemap->SetTile(0U, 0U, 0U, 1U);
    map->AddComponent<Spark::TilemapGameplayGridComponent>()->SetWalkRule(Spark::TilemapGameplayWalkRule::OccupiedWalkable);

    Spark::TilemapEditSession session{};
    ASSERT_TRUE(session.Attach(*map));
    Spark::TilemapBrush brush{};
    brush.mode = Spark::TilemapBrush::Mode::Single;
    brush.single = Spark::TileCell::FromTileId(2U);
    session.SetBrush(brush);
    EXPECT_TRUE(session.PaintCell(1U, 1U));

    Spark::TilemapDerivedDataRebaker::Options options{};
    options.autotile = false;
    const Spark::TilemapDerivedDataRebaker::Result result = session.RebakeDerivedDataForLastEdit(*map, options);
    EXPECT_TRUE(result.DidRebakeGameplayGrid());
    const Spark::TilemapGameplayGridComponent* grid = map->GetComponent<Spark::TilemapGameplayGridComponent>();
    ASSERT_NE(grid, nullptr);
    EXPECT_FALSE(grid->GetWalkability().IsWalkable(1, 1));
}

TEST(TilemapDerivedDataRebake, PartialAutotileRebakeUpdatesNeighbor) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeAutotileTileset(world), 3U, 3U, 1.0F, 0);
    tilemap->SetPaintTile(0U, 1U, 1U, 10U);
    map->AddComponent<Spark::TilemapAutotileComponent>();

    Spark::TilemapEditRevision revision{};
    revision.kind = Spark::TilemapEditChangeKind::CellPaint;
    revision.dirtyRegion = Spark::TilemapCellRegion::SingleCell(0U, 1U, 1U);

    Spark::TilemapDerivedDataRebaker rebaker{};
    Spark::TilemapDerivedDataRebaker::Options options{};
    options.gameplayGrid = false;
    const Spark::TilemapDerivedDataRebaker::Result result = rebaker.RebakeForRevision(*map, revision, options);
    EXPECT_TRUE(result.DidRebakeAutotile());
    EXPECT_EQ(tilemap->GetTileCell(0U, 1U, 1U).tileId, 20U);

    tilemap->SetPaintTile(0U, 2U, 1U, 10U);
    revision.dirtyRegion = Spark::TilemapCellRegion::SingleCell(0U, 2U, 1U);
    static_cast<void>(rebaker.RebakeForRevision(*map, revision, options));
    EXPECT_EQ(tilemap->GetTileCell(0U, 1U, 1U).tileId, 21U);
}
