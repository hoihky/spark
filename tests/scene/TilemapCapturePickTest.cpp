#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapMapSourceComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapObjectLayerComponent.hpp"
#include "spark/memory/SharedPtr.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/tilemap/TilemapDocumentCapture.hpp"
#include "spark/scene/tilemap/TilemapDocumentSerializer.hpp"
#include "spark/scene/tilemap/TilemapEditRevision.hpp"
#include "spark/scene/tilemap/TilemapLayerSortMode.hpp"
#include "spark/scene/tilemap/TilemapPick.hpp"

namespace {

Spark::SharedPtr<Spark::Tileset> MakeTestTileset(Spark::GameWorld& world) {
    Spark::Texture2D placeholder{};
    placeholder.GetName() = Spark::Utf8String("tests/tilemap_capture_atlas");
    const Spark::SharedPtr<Spark::Texture2D> atlas = world.RegisterTexture(
            Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(placeholder)), "tests/tilemap_capture_atlas");
    return Spark::MakeShared<Spark::Tileset>(atlas, 4U, 4U);
}

}  // namespace

TEST(TilemapCapturePick, CaptureRoundTripsLayersAndSortMode) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    map->AddComponent<Spark::TransformComponent>()->SetTranslation({2.0F, 3.0F, 0.0F});
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeTestTileset(world), 4U, 3U, 1.0F, 5);
    static_cast<void>(tilemap->AddLayer("Foreground"));
    tilemap->GetLayer(1U).sortMode = Spark::TilemapLayerSortMode::WorldY;
    tilemap->SetTile(0U, 1U, 1U, 2U);
    tilemap->SetTile(1U, 1U, 1U, 7U);

    auto* source = map->AddComponent<Spark::TilemapMapSourceComponent>();
    source->SetSparkMapPath("maps/demo.sparkmap");

    const Spark::TilemapDocumentCapturer capturer{};
    const Spark::TilemapDocumentCapturer::Result captured = capturer.CaptureFromOwner(*map);
    ASSERT_TRUE(captured.IsSuccess());
    EXPECT_EQ(captured.GetDocument().mapWidth, 4U);
    EXPECT_EQ(captured.GetDocument().mapHeight, 3U);
    EXPECT_EQ(captured.GetDocument().sortOrderBase, 5);
    EXPECT_EQ(captured.GetDocument().sourceTmxPath, Spark::Utf8String("maps/demo.sparkmap"));
    EXPECT_EQ(captured.GetDocument().tileLayers.GetSize(), 2U);
    EXPECT_EQ(captured.GetDocument().tileLayers[1U].sortMode, Spark::TilemapLayerSortMode::WorldY);
    EXPECT_EQ(captured.GetDocument().tileLayers[0U].cells[1U + 1U * 4U].tileId, 2U);
    EXPECT_EQ(captured.GetDocument().tileLayers[1U].cells[1U + 1U * 4U].tileId, 7U);
    EXPECT_FALSE(captured.GetDocument().tilesets.IsEmpty());
    EXPECT_EQ(captured.GetDocument().tilesets[0U].imagePath, Spark::Utf8String("tests/tilemap_capture_atlas"));
}

TEST(TilemapCapturePick, PickTopmostAndObjectMarker) {
    Spark::GameWorld world{};
    Spark::GameObject* map = world.CreateGameObject();
    Spark::TilemapComponent* tilemap = map->AddComponent<Spark::TilemapComponent>(MakeTestTileset(world), 4U, 4U, 1.0F, 0);
    static_cast<void>(tilemap->AddLayer("Top"));
    tilemap->SetTile(0U, 2U, 2U, 1U);
    tilemap->SetTile(1U, 2U, 2U, 9U);

    auto* objects = map->AddComponent<Spark::TilemapObjectLayerComponent>();
    static_cast<void>(objects->AddObjectLayer("Spawns"));
    Spark::TilemapObjectMarker marker{};
    marker.cellX = 2;
    marker.cellY = 2;
    marker.offsetX = 0.5F;
    marker.offsetY = 0.5F;
    const std::uint32_t markerId = objects->AddMarker(0U, marker);

    const Spark::TilemapPicker picker{};
    const Spark::TilemapCellPick top = picker.PickTopmostTile(*map, *tilemap, {2.5F, 2.5F});
    EXPECT_TRUE(top.IsHit());
    EXPECT_EQ(top.GetLayerIndex(), 1U);
    EXPECT_EQ(top.GetTile().tileId, 9U);

    const Spark::TilemapCellPick layer0 = picker.PickCellOnLayer(*map, *tilemap, 0U, {2.5F, 2.5F});
    EXPECT_TRUE(layer0.IsHit());
    EXPECT_EQ(layer0.GetTile().tileId, 1U);

    const Spark::TilemapObjectPick obj = picker.PickNearestObjectMarker(*map, *tilemap, *objects, {2.5F, 2.5F}, 0.6F);
    EXPECT_TRUE(obj.IsHit());
    EXPECT_EQ(obj.GetMarkerId(), markerId);
}

TEST(TilemapCapturePick, RevisionTrackerMergesDirtyRegion) {
    Spark::TilemapRevisionTracker tracker{};
    const Spark::TilemapEditRevision a = tracker.RecordCellChange(0U, 1U, 2U);
    EXPECT_EQ(a.revision, 1U);
    EXPECT_EQ(a.dirtyRegion.minX, 1U);
    EXPECT_EQ(a.dirtyRegion.maxY, 2U);

    const Spark::TilemapEditRevision b = tracker.RecordCellChange(0U, 3U, 4U);
    EXPECT_EQ(b.revision, 2U);
    EXPECT_EQ(b.dirtyRegion.minX, 1U);
    EXPECT_EQ(b.dirtyRegion.maxX, 3U);
    EXPECT_EQ(b.dirtyRegion.maxY, 4U);
}

TEST(TilemapCapturePick, SerializerRoundTripsSortMode) {
    Spark::TilemapDocument document{};
    document.mapWidth = 2U;
    document.mapHeight = 2U;
    Spark::TilemapDocumentTileLayer layer{};
    layer.name = Spark::Utf8String("L0");
    layer.sortMode = Spark::TilemapLayerSortMode::WorldY;
    layer.cells.Resize(4U);
    document.tileLayers.PushBack(layer);

    Spark::Utf8String text{};
    Spark::TilemapDocumentSerializer serializer{};
    ASSERT_TRUE(serializer.WriteToString(document, text));

    Spark::TilemapDocument parsed{};
    ASSERT_TRUE(serializer.ReadFromString(text.CStr(), parsed));
    EXPECT_EQ(parsed.tileLayers.GetSize(), 1U);
    EXPECT_EQ(parsed.tileLayers[0U].sortMode, Spark::TilemapLayerSortMode::WorldY);
}
