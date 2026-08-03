#include <gtest/gtest.h>

#include "spark/math/Vector3.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/core/PhysicsCore.hpp"

namespace {

Spark::CollisionAabb3 MakeBox(
        const float minX,
        const float minY,
        const float minZ,
        const float maxX,
        const float maxY,
        const float maxZ) {
    Spark::CollisionAabb3 box{};
    box.minX = minX;
    box.minY = minY;
    box.minZ = minZ;
    box.maxX = maxX;
    box.maxY = maxY;
    box.maxZ = maxZ;
    return box;
}

}  // namespace

TEST(Collision3DMath, AabbOverlapSeparated) {
    const Spark::CollisionAabb3 a = MakeBox(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
    const Spark::CollisionAabb3 b = MakeBox(2.0F, 0.0F, 0.0F, 3.0F, 1.0F, 1.0F);
    EXPECT_FALSE(Spark::CollisionAabb3Overlaps(a, b));
}

TEST(Collision3DMath, AabbOverlapPenetrating) {
    const Spark::CollisionAabb3 a = MakeBox(0.0F, 0.0F, 0.0F, 2.0F, 2.0F, 2.0F);
    const Spark::CollisionAabb3 b = MakeBox(1.0F, 1.0F, 1.0F, 3.0F, 3.0F, 3.0F);
    EXPECT_TRUE(Spark::CollisionAabb3Overlaps(a, b));
}

TEST(Collision3DMath, AabbOverlapsSphereInside) {
    const Spark::CollisionAabb3 box = MakeBox(0.0F, 0.0F, 0.0F, 4.0F, 4.0F, 4.0F);
    const Spark::Vector3 center{2.0F, 2.0F, 2.0F};
    EXPECT_TRUE(Spark::CollisionAabb3OverlapsSphere(box, center, 1.0F));
}

TEST(Collision3DMath, AabbOverlapsSphereOutside) {
    const Spark::CollisionAabb3 box = MakeBox(0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 1.0F);
    const Spark::Vector3 center{5.0F, 5.0F, 5.0F};
    EXPECT_FALSE(Spark::CollisionAabb3OverlapsSphere(box, center, 0.5F));
}

TEST(Collision3DMath, SphereAabbContactPenetrating) {
    const Spark::Vector3 center{0.5F, 0.0F, 0.0F};
    const Spark::CollisionAabb3 box = MakeBox(0.0F, -1.0F, -1.0F, 1.0F, 1.0F, 1.0F);
    float nx = 0.0F;
    float ny = 0.0F;
    float nz = 0.0F;
    float pen = 0.0F;
    const bool hit = Spark::ComputeSphereAabbContact(center, 1.0F, box, nx, ny, nz, pen);
    ASSERT_TRUE(hit);
    EXPECT_GT(pen, 0.0F);
    EXPECT_GT(nx, 0.0F);
}

TEST(Collision3DMath, SeparateSphereFromAabb) {
    Spark::Vector3 center{0.5F, 0.0F, 0.0F};
    const Spark::CollisionAabb3 box = MakeBox(0.0F, -1.0F, -1.0F, 1.0F, 1.0F, 1.0F);
    ASSERT_TRUE(Spark::SeparateSphereFromAabb(center, 1.0F, box));
    EXPECT_GE(center.x, 1.0F - 1.0e-4F);
}

TEST(Collision3DMath, StaticColliderMaterialRoundTrip) {
    Spark::StaticCollider3DSim collider{};
    collider.hasMaterial = true;
    collider.restitution = 0.3F;
    collider.staticFriction = 0.7F;
    collider.dynamicFriction = 0.5F;

    const Spark::ColliderMaterial material = Spark::ColliderMaterial::FromStaticCollider3D(collider);
    EXPECT_TRUE(material.isDefined);
    EXPECT_NEAR(material.restitution, 0.3F, 1.0e-5F);
    EXPECT_NEAR(material.staticFriction, 0.7F, 1.0e-5F);
    EXPECT_NEAR(material.dynamicFriction, 0.5F, 1.0e-5F);
}

TEST(PhysicsCoreTypes, ContactManifoldDefaults) {
    Spark::ContactManifold2D manifold2D{};
    EXPECT_FALSE(manifold2D.HasContact());

    Spark::ContactManifold3D manifold3D{};
    EXPECT_FALSE(manifold3D.HasContact());
}

TEST(PhysicsCoreTypes, RayDefaults) {
    const Spark::Ray2D ray2D{};
    EXPECT_NEAR(ray2D.origin.x, 0.0F, 1.0e-6F);
    EXPECT_NEAR(ray2D.direction.x, 1.0F, 1.0e-6F);

    const Spark::Ray3D ray3D{};
    EXPECT_NEAR(ray3D.origin.x, 0.0F, 1.0e-6F);
    EXPECT_NEAR(ray3D.direction.x, 1.0F, 1.0e-6F);
}

TEST(PhysicsCoreTypes, ColliderFilterMutualMask) {
    Spark::ColliderFilter a{};
    a.categoryBits = 0x1u;
    a.maskBits = 0x2u;

    Spark::ColliderFilter b{};
    b.categoryBits = 0x2u;
    b.maskBits = 0x1u;

    EXPECT_TRUE(a.ShouldCollideWith(b));
    EXPECT_TRUE(b.ShouldCollideWith(a));

    b.maskBits = 0x4u;
    EXPECT_FALSE(a.ShouldCollideWith(b));
}
