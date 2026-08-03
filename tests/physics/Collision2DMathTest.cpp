#include <gtest/gtest.h>

#include "spark/physics/Collision2D.hpp"
#include "spark/physics/core/PhysicsCore.hpp"

using namespace Spark;

namespace {

CollisionAabb2 MakeBox(const float minX, const float minY, const float maxX, const float maxY) {
    CollisionAabb2 box{};
    box.minX = minX;
    box.minY = minY;
    box.maxX = maxX;
    box.maxY = maxY;
    return box;
}

}  // namespace

TEST(Collision2DMath, AabbOverlapSeparated) {
    const CollisionAabb2 a = MakeBox(0.0F, 0.0F, 1.0F, 1.0F);
    const CollisionAabb2 b = MakeBox(2.0F, 0.0F, 3.0F, 1.0F);
    EXPECT_FALSE(Spark::CollisionAabb2Overlaps(a, b));
}

TEST(Collision2DMath, AabbOverlapTouching) {
    const CollisionAabb2 a = MakeBox(0.0F, 0.0F, 1.0F, 1.0F);
    const CollisionAabb2 b = MakeBox(1.0F, 0.0F, 2.0F, 1.0F);
    EXPECT_FALSE(Spark::CollisionAabb2Overlaps(a, b));
}

TEST(Collision2DMath, AabbOverlapPenetrating) {
    const CollisionAabb2 a = MakeBox(0.0F, 0.0F, 2.0F, 2.0F);
    const CollisionAabb2 b = MakeBox(1.0F, 1.0F, 3.0F, 3.0F);
    EXPECT_TRUE(Spark::CollisionAabb2Overlaps(a, b));
}

TEST(Collision2DMath, AabbOverlapsCircleInside) {
    const CollisionAabb2 box = MakeBox(0.0F, 0.0F, 4.0F, 4.0F);
    EXPECT_TRUE(Spark::CollisionAabb2OverlapsCircle(box, 2.0F, 2.0F, 1.0F));
}

TEST(Collision2DMath, AabbOverlapsCircleOutside) {
    const CollisionAabb2 box = MakeBox(0.0F, 0.0F, 1.0F, 1.0F);
    EXPECT_FALSE(Spark::CollisionAabb2OverlapsCircle(box, 5.0F, 5.0F, 0.5F));
}

TEST(Collision2DMath, CirclesOverlapTouching) {
    EXPECT_TRUE(Spark::CollisionCirclesOverlap(0.0F, 0.0F, 1.0F, 2.0F, 0.0F, 1.0F));
}

TEST(Collision2DMath, CirclesOverlapSeparated) {
    EXPECT_FALSE(Spark::CollisionCirclesOverlap(0.0F, 0.0F, 0.5F, 3.0F, 0.0F, 0.5F));
}

TEST(Collision2DMath, RaycastAabbHit) {
    const CollisionAabb2 box = MakeBox(1.0F, -1.0F, 2.0F, 1.0F);
    float hitT = -1.0F;
    const bool hit = Spark::RaycastSegmentAabb2(0.0F, 0.0F, 1.0F, 0.0F, 10.0F, box, hitT);
    ASSERT_TRUE(hit);
    EXPECT_NEAR(hitT, 1.0F, 1.0e-4F);
}

TEST(Collision2DMath, RaycastAabbMiss) {
    const CollisionAabb2 box = MakeBox(1.0F, 2.0F, 2.0F, 3.0F);
    float hitT = -1.0F;
    EXPECT_FALSE(Spark::RaycastSegmentAabb2(0.0F, 0.0F, 1.0F, 0.0F, 10.0F, box, hitT));
}

TEST(Collision2DMath, RaycastCircleHit) {
    float hitT = -1.0F;
    const bool hit = Spark::RaycastSegmentCircle2(0.0F, 0.0F, 1.0F, 0.0F, 10.0F, 5.0F, 0.0F, 1.0F, hitT);
    ASSERT_TRUE(hit);
    EXPECT_GT(hitT, 3.9F);
    EXPECT_LT(hitT, 4.1F);
}

TEST(Collision2DMath, StaticColliderFilterRoundTrip) {
    Spark::StaticCollider2D collider{};
    collider.categoryBits = 0x4u;
    collider.maskBits = 0x2u;
    collider.isTrigger = true;
    collider.hasMaterial = true;
    collider.restitution = 0.25F;
    collider.dynamicFriction = 0.6F;

    const Spark::ColliderFilter filter = Spark::ColliderFilter::FromStaticCollider2D(collider);
    EXPECT_EQ(filter.categoryBits, 0x4u);
    EXPECT_EQ(filter.maskBits, 0x2u);
    EXPECT_TRUE(filter.isTrigger);

    const Spark::ColliderMaterial material = Spark::ColliderMaterial::FromStaticCollider2D(collider);
    EXPECT_TRUE(material.isDefined);
    EXPECT_NEAR(material.restitution, 0.25F, 1.0e-5F);
    EXPECT_NEAR(material.dynamicFriction, 0.6F, 1.0e-5F);
}
