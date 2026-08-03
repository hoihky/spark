#include <gtest/gtest.h>

#include "spark/physics/colliders/DynamicCollider2D.hpp"
#include "spark/physics/colliders/DynamicCollider3D.hpp"

TEST(DynamicColliderTest, Collider2DLegacyRoundTripPreservesBounds) {
    Spark::DynamicCollider2DSim legacy{};
    legacy.shape = Spark::DynamicCollider2DShape::Circle;
    legacy.circleCx = 1.0F;
    legacy.circleCy = 2.0F;
    legacy.circleR = 0.75F;
    legacy.aabb = {0.25F, 1.25F, 1.75F, 2.75F};

    const Spark::DynamicCollider2D collider = Spark::DynamicCollider2D::FromLegacySnapshot(legacy);
    EXPECT_TRUE(collider.IsValid());
    EXPECT_TRUE(collider.OverlapsCircle(1.0F, 2.0F, 0.5F));

    const Spark::DynamicCollider2DSim roundTrip = collider.ToLegacySnapshot();
    EXPECT_EQ(roundTrip.shape, Spark::DynamicCollider2DShape::Circle);
    EXPECT_FLOAT_EQ(roundTrip.circleR, legacy.circleR);
}

TEST(DynamicColliderTest, Collider3DLegacyRoundTripPreservesTrackingPoint) {
    Spark::DynamicCollider3DSim legacy{};
    legacy.shape = Spark::DynamicCollider3DShape::Sphere;
    legacy.sphereCenter = {1.0F, 2.0F, 3.0F};
    legacy.sphereRadius = 0.5F;
    legacy.bounds = {0.5F, 1.5F, 2.5F, 1.5F, 2.5F, 3.5F};

    Spark::DynamicCollider3D collider = Spark::DynamicCollider3D::FromLegacySnapshot(legacy);
    EXPECT_TRUE(collider.IsValid());
    const Spark::Vector3 tracking = collider.GetTrackingPoint();
    EXPECT_FLOAT_EQ(tracking.x, legacy.sphereCenter.x);
    EXPECT_FLOAT_EQ(tracking.y, legacy.sphereCenter.y);
    EXPECT_FLOAT_EQ(tracking.z, legacy.sphereCenter.z);

    collider.Translate({1.0F, 0.0F, 0.0F});
    const Spark::Vector3 movedTracking = collider.GetTrackingPoint();
    EXPECT_FLOAT_EQ(movedTracking.x, tracking.x + 1.0F);
}

TEST(DynamicColliderTest, DynamicCollider2DOverlapsBoxBox) {
    Spark::DynamicCollider2DSim legacyA{};
    legacyA.shape = Spark::DynamicCollider2DShape::Box;
    legacyA.aabb = {0.0F, 0.0F, 2.0F, 2.0F};
    Spark::DynamicCollider2DSim legacyB{};
    legacyB.shape = Spark::DynamicCollider2DShape::Box;
    legacyB.aabb = {1.0F, 1.0F, 3.0F, 3.0F};

    const Spark::DynamicCollider2D a = Spark::DynamicCollider2D::FromLegacySnapshot(legacyA);
    const Spark::DynamicCollider2D b = Spark::DynamicCollider2D::FromLegacySnapshot(legacyB);
    EXPECT_TRUE(a.Overlaps(b));
}
