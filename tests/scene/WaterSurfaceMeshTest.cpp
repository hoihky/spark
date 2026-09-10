#include <gtest/gtest.h>

#include "spark/scene/water/WaterSurfaceMesh.hpp"

namespace {

TEST(WaterSurfaceMeshTest, SubdivisionsProduceExpectedTopology) {
    const std::size_t vertexCount = Spark::WaterSurfaceMesh::VertexCountForSubdivisions(4);
    const std::size_t indexCount = Spark::WaterSurfaceMesh::IndexCountForSubdivisions(4);
    EXPECT_EQ(vertexCount, 25U);
    EXPECT_EQ(indexCount, 96U);
}

TEST(WaterSurfaceMeshTest, RebuildThresholdHonored) {
    Spark::WaterSurfaceMeshSettings settings{};
    settings.SetTileHalfExtent(64.0F);
    settings.SetRebuildMoveThreshold(10.0F);
    Spark::WaterSurfaceMesh surface(settings);

    surface.RebuildTile(64.0F, 64.0F, {0.0F, 0.0F});
    EXPECT_FALSE(surface.ShouldRebuildForCamera({5.0F, 0.0F, 5.0F}));
    EXPECT_TRUE(surface.ShouldRebuildForCamera({20.0F, 0.0F, 0.0F}));
}

TEST(WaterSurfaceMeshTest, SnapAnchorToTileGrid) {
    Spark::WaterSurfaceMeshSettings settings{};
    settings.SetTileHalfExtent(50.0F);
    Spark::WaterSurfaceMesh surface(settings);

    const Spark::Vector2 anchor = surface.ComputeSnappedAnchorXZ({123.0F, 0.0F, 77.0F});
    EXPECT_FLOAT_EQ(anchor.x, 150.0F);
    EXPECT_FLOAT_EQ(anchor.y, 50.0F);
}

TEST(WaterSurfaceMeshTest, RebuildBumpsGeometryRevision) {
    Spark::WaterSurfaceMesh surface({});
    const std::uint32_t revisionBefore = surface.GetMesh()->GetGeometryRevision();
    surface.RebuildTile(8.0F, 8.0F, {0.0F, 0.0F});
    EXPECT_GT(surface.GetMesh()->GetGeometryRevision(), revisionBefore);
}

}  // namespace
