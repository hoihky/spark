#include <gtest/gtest.h>

#include "spark/math/Vector3.hpp"
#include "spark/scene/foliage/GrassChunkCoordinate.hpp"
#include "spark/scene/foliage/GrassChunkScatterBuilder.hpp"
#include "spark/scene/foliage/GrassFieldBounds.hpp"

TEST(GrassFieldScatterTest, ChunkCoordinateFromNegativeWorldPosition) {
    const Spark::GrassChunkCoordinate chunk =
            Spark::GrassChunkCoordinate::FromWorldPosition(-8.0F, 12.0F, 32.0F);
    EXPECT_EQ(chunk.GetIndexX(), -1);
    EXPECT_EQ(chunk.GetIndexZ(), 0);
}

TEST(GrassFieldScatterTest, ChunkWorldMinAtNegativeIndex) {
    const Spark::GrassChunkCoordinate chunk{-2, 1};
    EXPECT_FLOAT_EQ(chunk.WorldMinX(32.0F), -64.0F);
    EXPECT_FLOAT_EQ(chunk.WorldMinZ(32.0F), 32.0F);
}

TEST(GrassFieldScatterTest, DistanceFadeBandFromViewRadius) {
    Spark::GrassChunkScatterSettings settings{};
    settings.SetMaxViewDistanceMeters(40.0F);
    settings.SetDistanceFadeOuterFraction(0.25F);
    EXPECT_FLOAT_EQ(settings.GetDistanceFadeEndMeters(), 40.0F);
    EXPECT_FLOAT_EQ(settings.GetDistanceFadeStartMeters(), 30.0F);
}

TEST(GrassFieldScatterTest, BoundsRejectOutsideHalfExtents) {
    Spark::GrassFieldBounds bounds{};
    bounds.SetHalfExtentsMeters(10.0F, 10.0F);
    const Spark::Vector3 origin{0.0F, 0.0F, 0.0F};
    EXPECT_TRUE(bounds.ContainsWorldXZ(origin, 9.0F, -9.0F));
    EXPECT_FALSE(bounds.ContainsWorldXZ(origin, 10.5F, 0.0F));
}
