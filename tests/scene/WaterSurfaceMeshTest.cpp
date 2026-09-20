#include <gtest/gtest.h>

#include "spark/scene/water/WaterSurfaceMesh.hpp"

namespace {

TEST(WaterSurfaceMeshTest, SubdivisionsProduceExpectedTopology) {
    const std::size_t vertexCount = Spark::WaterSurfaceMesh::VertexCountForSubdivisions(4);
    const std::size_t indexCount = Spark::WaterSurfaceMesh::IndexCountForSubdivisions(4);
    EXPECT_EQ(vertexCount, 25U);
    EXPECT_EQ(indexCount, 96U);
}

TEST(WaterSurfaceMeshTest, RebuildWhenSnappedAnchorChanges) {
    Spark::WaterSurfaceMeshSettings settings{};
    settings.SetTileHalfExtent(50.0F);
    Spark::WaterSurfaceMesh surface(settings);

    surface.RebuildTile(50.0F, 50.0F, {0.0F, 0.0F});
    EXPECT_FALSE(surface.ShouldRebuildForCamera({10.0F, 0.0F, 10.0F}));
    EXPECT_TRUE(surface.ShouldRebuildForCamera({120.0F, 0.0F, 10.0F}));
}

TEST(WaterSurfaceMeshTest, SnapAnchorCentersOnWorldOrigin) {
    Spark::WaterSurfaceMeshSettings settings{};
    settings.SetTileHalfExtent(128.0F);
    Spark::WaterSurfaceMesh surface(settings);

    const Spark::Vector2 nearIsland = surface.ComputeSnappedAnchorXZ({58.0F, 0.0F, 22.0F});
    const Spark::Vector2 oppositeSide = surface.ComputeSnappedAnchorXZ({-42.0F, 0.0F, -18.0F});
    EXPECT_FLOAT_EQ(nearIsland.x, 0.0F);
    EXPECT_FLOAT_EQ(nearIsland.y, 0.0F);
    EXPECT_FLOAT_EQ(oppositeSide.x, 0.0F);
    EXPECT_FLOAT_EQ(oppositeSide.y, 0.0F);
}

TEST(WaterSurfaceMeshTest, SnapAnchorToTileGrid) {
    Spark::WaterSurfaceMeshSettings settings{};
    settings.SetTileHalfExtent(50.0F);
    Spark::WaterSurfaceMesh surface(settings);

    const Spark::Vector2 anchor = surface.ComputeSnappedAnchorXZ({123.0F, 0.0F, 77.0F});
    EXPECT_FLOAT_EQ(anchor.x, 100.0F);
    EXPECT_FLOAT_EQ(anchor.y, 100.0F);
}

TEST(WaterSurfaceMeshTest, RebuildBumpsGeometryRevision) {
    Spark::WaterSurfaceMesh surface({});
    const std::uint32_t revisionBefore = surface.GetMesh()->GetGeometryRevision();
    surface.RebuildTile(8.0F, 8.0F, {0.0F, 0.0F});
    EXPECT_GT(surface.GetMesh()->GetGeometryRevision(), revisionBefore);
}

}  // namespace
