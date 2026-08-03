#include <gtest/gtest.h>

#include "spark/memory/UniquePtr.hpp"
#include "spark/physics/Collision2D.hpp"
#include "spark/physics/Collision3D.hpp"
#include "spark/physics/shapes/PhysicsShapes.hpp"

using namespace Spark;

namespace {

CollisionAabb2 MakeBox2D(const float minX, const float minY, const float maxX, const float maxY) {
    CollisionAabb2 box{};
    box.minX = minX;
    box.minY = minY;
    box.maxX = maxX;
    box.maxY = maxY;
    return box;
}

CollisionAabb3 MakeBox3D(
        const float minX,
        const float minY,
        const float minZ,
        const float maxX,
        const float maxY,
        const float maxZ) {
    CollisionAabb3 box{};
    box.minX = minX;
    box.minY = minY;
    box.minZ = minZ;
    box.maxX = maxX;
    box.maxY = maxY;
    box.maxZ = maxZ;
    return box;
}

}  // namespace

TEST(ShapeNarrowPhase2D, FactoryFromStaticColliderMatchesLegacyOverlap) {
    Spark::StaticCollider2D boxStatic{};
    boxStatic.shape = Spark::StaticCollider2DShape::Box;
    boxStatic.aabb = MakeBox2D(0.0F, 0.0F, 2.0F, 2.0F);

    Spark::StaticCollider2D circleStatic{};
    circleStatic.shape = Spark::StaticCollider2DShape::Circle;
    circleStatic.circleCx = 1.0F;
    circleStatic.circleCy = 1.0F;
    circleStatic.circleR = 0.5F;

    const Spark::UniquePtr<Spark::IShape2D> boxShape = Spark::ShapeFactory2D::CreateFromStaticCollider(boxStatic);
    const Spark::UniquePtr<Spark::IShape2D> circleShape = Spark::ShapeFactory2D::CreateFromStaticCollider(circleStatic);
    ASSERT_TRUE(boxShape);
    ASSERT_TRUE(circleShape);

    const CollisionAabb2 query = MakeBox2D(0.5F, 0.5F, 1.5F, 1.5F);
    EXPECT_EQ(boxShape->OverlapsAabb(query), Spark::StaticCollider2DOverlapsWorldAabb(boxStatic, query));
    EXPECT_EQ(circleShape->OverlapsAabb(query), Spark::StaticCollider2DOverlapsWorldAabb(circleStatic, query));
    EXPECT_EQ(
            Spark::NarrowPhase2D::Overlap(*boxShape, *circleShape),
            Spark::StaticCollider2DOverlapsWorldCircle(boxStatic, circleStatic.circleCx, circleStatic.circleCy, circleStatic.circleR));
}

TEST(ShapeNarrowPhase2D, CircleCircleContact) {
    const Spark::UniquePtr<Spark::IShape2D> a = Spark::ShapeFactory2D::CreateCircle(0.0F, 0.0F, 1.0F);
    const Spark::UniquePtr<Spark::IShape2D> b = Spark::ShapeFactory2D::CreateCircle(1.5F, 0.0F, 1.0F);
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);

    Spark::ContactManifold2D manifold{};
    EXPECT_TRUE(Spark::NarrowPhase2D::ComputeContact(*a, *b, manifold));
    EXPECT_TRUE(manifold.HasContact());
    EXPECT_GT(manifold.penetration, 0.0F);
}

TEST(ShapeNarrowPhase3D, FactorySphereBoxOverlapMatchesLegacy) {
    Spark::StaticCollider3DSim boxStatic{};
    boxStatic.shape = Spark::StaticCollider3DShape::Box;
    boxStatic.aabb = MakeBox3D(0.0F, 0.0F, 0.0F, 2.0F, 2.0F, 2.0F);

    const Spark::Vector3 center{1.0F, 1.0F, 1.0F};
    const float radius = 0.75F;

    const Spark::UniquePtr<Spark::IShape3D> boxShape = Spark::ShapeFactory3D::CreateFromStaticCollider(boxStatic);
    const Spark::UniquePtr<Spark::IShape3D> sphereShape = Spark::ShapeFactory3D::CreateSphere(center, radius);
    ASSERT_TRUE(boxShape);
    ASSERT_TRUE(sphereShape);

    EXPECT_EQ(Spark::StaticCollider3DOverlapsSphere(boxStatic, center, radius), true);
    EXPECT_EQ(Spark::NarrowPhase3D::Overlap(*boxShape, *sphereShape), true);
}

TEST(ShapeNarrowPhase3D, SphereSphereContact) {
    const Spark::UniquePtr<Spark::IShape3D> a = Spark::ShapeFactory3D::CreateSphere({0.0F, 0.0F, 0.0F}, 1.0F);
    const Spark::UniquePtr<Spark::IShape3D> b = Spark::ShapeFactory3D::CreateSphere({1.5F, 0.0F, 0.0F}, 1.0F);
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);

    Spark::ContactManifold3D manifold{};
    EXPECT_TRUE(Spark::NarrowPhase3D::ComputeContact(*a, *b, manifold));
    EXPECT_TRUE(manifold.HasContact());
}

TEST(ShapeTypes, UniquePtrPolymorphism) {
    Spark::UniquePtr<Spark::IShape2D> shape = Spark::ShapeFactory2D::CreateBox(MakeBox2D(0.0F, 0.0F, 1.0F, 1.0F));
    ASSERT_TRUE(shape);
    EXPECT_EQ(shape->GetType(), Spark::ShapeType2D::Box);

    Spark::UniquePtr<Spark::IShape3D> shape3D =
            Spark::ShapeFactory3D::CreateSphere(Spark::Vector3::Zero, 0.5F);
    ASSERT_TRUE(shape3D);
    EXPECT_EQ(shape3D->GetType(), Spark::ShapeType3D::Sphere);
}
