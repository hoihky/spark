#include <gtest/gtest.h>

#include "spark/physics/colliders/Collider2D.hpp"
#include "spark/physics/colliders/Collider3D.hpp"
#include "spark/physics/shapes/ShapeFactory2D.hpp"
#include "spark/physics/shapes/ShapeFactory3D.hpp"

TEST(ColliderSnapshotTest, Collider2DLegacyRoundTripPreservesOverlap) {
    Spark::StaticCollider2D legacy{};
    legacy.shape = Spark::StaticCollider2DShape::Circle;
    legacy.circleCx = 2.0F;
    legacy.circleCy = 3.0F;
    legacy.circleR = 1.5F;
    legacy.categoryBits = 0x4;
    legacy.maskBits = 0x8;
    legacy.isTrigger = true;
    legacy.hasMaterial = true;
    legacy.restitution = 0.25F;
    legacy.dynamicFriction = 0.4F;
    legacy.aabb = {0.5F, 1.5F, 3.5F, 4.5F};

    const Spark::Collider2D collider = Spark::Collider2D::FromLegacySnapshot(legacy);
    EXPECT_TRUE(collider.IsValid());
    EXPECT_EQ(collider.GetCategoryBits(), legacy.categoryBits);
    EXPECT_EQ(collider.GetMaskBits(), legacy.maskBits);
    EXPECT_EQ(collider.IsTrigger(), legacy.isTrigger);
    EXPECT_TRUE(collider.GetMaterial().isDefined);
    EXPECT_FLOAT_EQ(collider.GetMaterial().restitution, legacy.restitution);

    const Spark::CollisionAabb2 query{1.0F, 2.0F, 4.0F, 5.0F};
    EXPECT_EQ(collider.OverlapsAabb(query), Spark::Collider2DOverlapsWorldAabb(collider, query));
    EXPECT_EQ(
            collider.OverlapsCircle(2.0F, 3.0F, 1.0F),
            Spark::Collider2DOverlapsWorldCircle(collider, 2.0F, 3.0F, 1.0F));

    const Spark::StaticCollider2D roundTrip = collider.ToLegacySnapshot();
    EXPECT_EQ(roundTrip.shape, Spark::StaticCollider2DShape::Circle);
    EXPECT_FLOAT_EQ(roundTrip.circleCx, legacy.circleCx);
    EXPECT_FLOAT_EQ(roundTrip.circleCy, legacy.circleCy);
    EXPECT_FLOAT_EQ(roundTrip.circleR, legacy.circleR);
}

TEST(ColliderSnapshotTest, Collider3DLegacyRoundTripPreservesOverlap) {
    Spark::StaticCollider3DSim legacy{};
    legacy.shape = Spark::StaticCollider3DShape::Capsule;
    legacy.capsule.pointA = {0.0F, 0.0F, 0.0F};
    legacy.capsule.pointB = {0.0F, 2.0F, 0.0F};
    legacy.capsule.radius = 0.5F;
    legacy.aabb = {-0.5F, -0.5F, -0.5F, 0.5F, 2.5F, 0.5F};
    legacy.hasMaterial = true;
    legacy.restitution = 0.1F;
    legacy.staticFriction = 0.6F;
    legacy.dynamicFriction = 0.5F;

    const Spark::Collider3D collider = Spark::Collider3D::FromLegacySnapshot(legacy);
    EXPECT_TRUE(collider.IsValid());
    EXPECT_TRUE(collider.GetMaterial().isDefined);

    const Spark::Vector3 probe{0.0F, 1.0F, 0.0F};
    EXPECT_EQ(collider.OverlapsSphere(probe, 0.25F), Spark::Collider3DOverlapsSphere(collider, probe, 0.25F));

    const Spark::StaticCollider3DSim roundTrip = collider.ToLegacySnapshot();
    EXPECT_EQ(roundTrip.shape, Spark::StaticCollider3DShape::Capsule);
    EXPECT_FLOAT_EQ(roundTrip.capsule.radius, legacy.capsule.radius);
}

TEST(ColliderSnapshotTest, Collider2DCreateFromShapeFactory) {
    Spark::CollisionAabb2 box{0.0F, 0.0F, 2.0F, 2.0F};
    Spark::UniquePtr<Spark::IShape2D> shape = Spark::ShapeFactory2D::CreateBox(box);
    Spark::ColliderFilter filter{};
    filter.categoryBits = 1;
    Spark::ColliderMaterial material{};
    material.isDefined = true;
    material.restitution = 0.2F;

    Spark::Collider2D collider = Spark::Collider2D::Create(MoveTemp(shape), filter, material, nullptr);
    EXPECT_TRUE(collider.IsValid());
    EXPECT_EQ(collider.GetShapeType(), Spark::ShapeType2D::Box);
    EXPECT_TRUE(collider.OverlapsAabb({0.5F, 0.5F, 1.5F, 1.5F}));
    EXPECT_FALSE(collider.OverlapsAabb({3.0F, 3.0F, 4.0F, 4.0F}));
}
