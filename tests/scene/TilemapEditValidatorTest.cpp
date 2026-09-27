#include <gtest/gtest.h>

#include "spark/scene/tilemap/TilemapEditValidator.hpp"

namespace {

Spark::TilemapDocument MakeMinimalDocument(const std::uint32_t width, const std::uint32_t height) {
    Spark::TilemapDocument document{};
    document.mapWidth = width;
    document.mapHeight = height;
    Spark::TilemapDocumentTileset tileset{};
    tileset.tileCount = 4U;
    tileset.columns = 2U;
    tileset.imagePath = Spark::Utf8String("spark/missing_atlas_for_test.png");
    document.tilesets.PushBack(tileset);
    Spark::TilemapDocumentTileLayer layer{};
    layer.name = Spark::Utf8String("Ground");
    layer.cells.Reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            layer.cells.PushBack(Spark::TileCell::FromTileId(1U));
        }
    }
    document.tileLayers.PushBack(layer);
    return document;
}

}  // namespace

TEST(TilemapEditValidator, CleanDocumentPasses) {
    const Spark::TilemapDocument document = MakeMinimalDocument(2U, 2U);
    Spark::TilemapEditValidator validator{};
    Spark::TilemapEditValidator::Options options{};
    options.checkAtlasFiles = false;
    const Spark::TilemapEditValidationReport report = validator.Validate(document, options);
    EXPECT_TRUE(report.IsClean());
}

TEST(TilemapEditValidator, OrphanTileIdDetected) {
    Spark::TilemapDocument document = MakeMinimalDocument(2U, 2U);
    document.tileLayers[0].cells[0] = Spark::TileCell::FromTileId(99U);
    Spark::TilemapEditValidator validator{};
    Spark::TilemapEditValidator::Options options{};
    options.checkAtlasFiles = false;
    const Spark::TilemapEditValidationReport report = validator.Validate(document, options);
    EXPECT_FALSE(report.IsClean());
    EXPECT_EQ(report.GetIssues()[0].GetKind(), Spark::TilemapEditIssueKind::OrphanTileId);
}

TEST(TilemapEditValidator, LayerCellCountMismatch) {
    Spark::TilemapDocument document = MakeMinimalDocument(2U, 2U);
    document.tileLayers[0].cells.PopBack();
    Spark::TilemapEditValidator validator{};
    Spark::TilemapEditValidator::Options options{};
    options.checkAtlasFiles = false;
    const Spark::TilemapEditValidationReport report = validator.Validate(document, options);
    EXPECT_FALSE(report.IsClean());
    EXPECT_EQ(report.GetIssues()[0].GetKind(), Spark::TilemapEditIssueKind::LayerCellCountMismatch);
}

TEST(TilemapEditValidator, ObjectMarkerOffMap) {
    Spark::TilemapDocument document = MakeMinimalDocument(2U, 2U);
    Spark::TilemapObjectLayer objectLayer{};
    objectLayer.name = Spark::Utf8String("Objects");
    Spark::TilemapObjectMarker marker{};
    marker.cellX = 5;
    marker.cellY = 0;
    objectLayer.markers.PushBack(marker);
    document.objectLayers.PushBack(objectLayer);
    Spark::TilemapEditValidator validator{};
    Spark::TilemapEditValidator::Options options{};
    options.checkAtlasFiles = false;
    const Spark::TilemapEditValidationReport report = validator.Validate(document, options);
    EXPECT_FALSE(report.IsClean());
    EXPECT_EQ(report.GetIssues()[0].GetKind(), Spark::TilemapEditIssueKind::ObjectMarkerOffMap);
}
