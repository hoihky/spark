#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/physics/CollisionFilter3D.hpp"
#include "spark/physics/PhysicsQueries3D.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/physics/PhysicsWorld3D.hpp"
#include "spark/physics/simulation/PhysicsFixedStepClock.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace {

Spark::GameObject* AddStaticFloor(Spark::GameWorld& world, const Spark::Vector3& center, const Spark::Vector3& halfExtents) {
    Spark::GameObject* floor = world.CreateGameObject();
    floor->AddComponent<Spark::TransformComponent>()->SetTranslation(center);
    auto* box = floor->AddComponent<Spark::BoxCollider3DComponent>(halfExtents);
    box->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Environment));
    box->SetMaskBits(Spark::CollisionFilter3D::AllLayersMask());
    return floor;
}

Spark::GameObject* AddDynamicSphere(
        Spark::GameWorld& world,
        const Spark::Vector3& position,
        float radius,
        const Spark::Vector3& velocity,
        std::uint16_t categoryBits) {
    Spark::GameObject* ball = world.CreateGameObject();
    ball->AddComponent<Spark::TransformComponent>()->SetTranslation(position);
    auto* sphere = ball->AddComponent<Spark::SphereCollider3DComponent>(radius);
    sphere->SetCategoryBits(categoryBits);
    sphere->SetMaskBits(Spark::CollisionFilter3D::AllLayersMask());
    auto* rb = ball->AddComponent<Spark::Rigidbody3DComponent>();
    rb->SetVelocity(velocity);
    return ball;
}

Spark::PhysicsQueryFilter3D DefaultSolidQueryFilter() {
    Spark::PhysicsQueryFilter3D filter{};
    filter.queryCategoryBits = Spark::CollisionFilter3D::DefaultCategory();
    filter.queryMaskBits = Spark::CollisionFilter3D::AllLayersMask();
    filter.hitSolids = true;
    filter.hitTriggers = false;
    return filter;
}

}  // namespace

TEST(PhysicsFixedStepClockTest, VariableModeUsesFrameDelta) {
    Spark::PhysicsFixedStepClock clock{};
    clock.SetFixedTimestepSeconds(0.0F);
    EXPECT_EQ(clock.BeginFrame(0.016F), 1);
    EXPECT_FLOAT_EQ(clock.GetStepDeltaSeconds(), 0.016F);
}

TEST(PhysicsFixedStepClockTest, FixedModeAccumulatesMultipleSteps) {
    Spark::PhysicsFixedStepClock clock{};
    clock.SetFixedTimestepSeconds(1.0F / 60.0F);
    clock.SetMaxStepsPerFrame(4);
    const int steps = clock.BeginFrame(0.05F + 1.0e-4F);
    EXPECT_GE(steps, 2);
    EXPECT_LE(steps, 3);
    EXPECT_FLOAT_EQ(clock.GetStepDeltaSeconds(), 1.0F / 60.0F);
    EXPECT_GT(clock.GetAccumulatorSeconds(), 0.0F);
}

TEST(PhysicsFixedStepClockTest, MaxStepsCapsSpiral) {
    Spark::PhysicsFixedStepClock clock{};
    clock.SetFixedTimestepSeconds(0.01F);
    clock.SetMaxStepsPerFrame(2);
    EXPECT_EQ(clock.BeginFrame(1.0F), 2);
}

TEST(PhysicsQueryWorld3DTest, OverlapSphereHitsStaticFloor) {
    Spark::GameWorld world{};
    AddStaticFloor(world, Spark::Vector3{0.0F, 0.0F, 0.0F}, Spark::Vector3{10.0F, 0.5F, 10.0F});

    Spark::PhysicsQueryWorld3D queries{};
    queries.RebuildStatics(world);

    Spark::Array<Spark::PhysicsQueryHit3D> hits{};
    queries.OverlapSphereStatics(Spark::Vector3{0.0F, 0.5F, 0.0F}, 0.25F, DefaultSolidQueryFilter(), hits);
    EXPECT_EQ(hits.GetSize(), 1u);
    EXPECT_NE(hits[0].owner, nullptr);
}

TEST(PhysicsQueryWorld3DTest, OverlapSphereFindsDynamicRigidbody) {
    Spark::GameWorld world{};
    AddDynamicSphere(
            world,
            Spark::Vector3{2.0F, 1.0F, 0.0F},
            0.5F,
            Spark::Vector3::Zero,
            Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Projectile));

    Spark::PhysicsQueryWorld3D queries{};
    queries.RebuildDynamics(world);

    Spark::Array<Spark::PhysicsQueryHitDynamic3D> hits{};
    queries.OverlapSphereDynamics(Spark::Vector3{2.0F, 1.0F, 0.0F}, 0.6F, DefaultSolidQueryFilter(), nullptr, hits);
    EXPECT_EQ(hits.GetSize(), 1u);
}

TEST(PhysicsQueryWorld3DTest, LayerMaskBlocksDynamicOverlap) {
    Spark::GameWorld world{};
    AddDynamicSphere(
            world,
            Spark::Vector3{0.0F, 1.0F, 0.0F},
            0.5F,
            Spark::Vector3::Zero,
            Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Enemy));

    Spark::PhysicsQueryFilter3D filter{};
    filter.queryCategoryBits = Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Player);
    filter.queryMaskBits = Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Environment);
    filter.hitSolids = true;

    Spark::PhysicsQueryWorld3D queries{};
    queries.RebuildDynamics(world);

    Spark::Array<Spark::PhysicsQueryHitDynamic3D> hits{};
    queries.OverlapSphereDynamics(Spark::Vector3{0.0F, 1.0F, 0.0F}, 1.0F, filter, nullptr, hits);
    EXPECT_EQ(hits.GetSize(), 0u);
}

TEST(PhysicsQueryWorld3DTest, IgnoreObjectSkipsOwner) {
    Spark::GameWorld world{};
    Spark::GameObject* ball = AddDynamicSphere(
            world,
            Spark::Vector3{0.0F, 2.0F, 0.0F},
            0.4F,
            Spark::Vector3::Zero,
            Spark::CollisionFilter3D::DefaultCategory());

    Spark::PhysicsQueryWorld3D queries{};
    queries.RebuildDynamics(world);

    Spark::Array<Spark::PhysicsQueryHitDynamic3D> hits{};
    queries.OverlapSphereDynamics(Spark::Vector3{0.0F, 2.0F, 0.0F}, 0.5F, DefaultSolidQueryFilter(), ball, hits);
    EXPECT_EQ(hits.GetSize(), 0u);
}

TEST(PhysicsQueryWorld3DTest, RaycastStaticsHitsBox) {
    Spark::GameWorld world{};
    AddStaticFloor(world, Spark::Vector3{0.0F, 0.0F, 0.0F}, Spark::Vector3{5.0F, 0.5F, 5.0F});

    Spark::PhysicsQueryWorld3D queries{};
    queries.RebuildStatics(world);

    Spark::PhysicsRaycastHit3D hit{};
    const bool ok = queries.RaycastStatics(
            Spark::Vector3{0.0F, 5.0F, 0.0F},
            Spark::Vector3{0.0F, -1.0F, 0.0F},
            20.0F,
            DefaultSolidQueryFilter(),
            hit);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(hit.isDynamicHit);
    EXPECT_NEAR(hit.distanceAlongRay, 4.5F, 0.05F);
}

TEST(PhysicsQueryWorld3DTest, RaycastDynamicsHitsSphere) {
    Spark::GameWorld world{};
    AddDynamicSphere(
            world,
            Spark::Vector3{0.0F, 3.0F, 0.0F},
            0.5F,
            Spark::Vector3::Zero,
            Spark::CollisionFilter3D::DefaultCategory());

    Spark::PhysicsQueryWorld3D queries{};
    queries.RebuildDynamics(world);

    Spark::PhysicsRaycastHit3D hit{};
    const bool ok = queries.RaycastDynamics(
            Spark::Vector3{0.0F, 8.0F, 0.0F},
            Spark::Vector3{0.0F, -1.0F, 0.0F},
            20.0F,
            DefaultSolidQueryFilter(),
            nullptr,
            hit);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(hit.isDynamicHit);
    EXPECT_EQ(hit.staticColliderIndex, Spark::PhysicsRaycastHit3D::kNoStaticColliderIndex);
    EXPECT_NEAR(hit.distanceAlongRay, 4.5F, 0.1F);
}

TEST(PhysicsQueryWorld3DTest, CombinedRaycastPicksNearest) {
    Spark::GameWorld world{};
    AddStaticFloor(world, Spark::Vector3{0.0F, 0.0F, 0.0F}, Spark::Vector3{10.0F, 0.5F, 10.0F});
    AddDynamicSphere(
            world,
            Spark::Vector3{0.0F, 2.0F, 0.0F},
            0.5F,
            Spark::Vector3::Zero,
            Spark::CollisionFilter3D::DefaultCategory());

    Spark::PhysicsQueryWorld3D queries{};
    Spark::PhysicsRaycastHit3D hit{};
    const bool ok = queries.Raycast(
            world,
            Spark::Vector3{0.0F, 5.0F, 0.0F},
            Spark::Vector3{0.0F, -1.0F, 0.0F},
            20.0F,
            DefaultSolidQueryFilter(),
            nullptr,
            hit);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(hit.isDynamicHit);
    EXPECT_LT(hit.distanceAlongRay, 3.5F);
}

TEST(PhysicsQueryWorld3DTest, CharacterControllerIncludedInDynamicCatalog) {
    Spark::GameWorld world{};
    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{1.0F, 0.0F, 0.0F});
    auto* cc = player->AddComponent<Spark::CharacterController3DComponent>(0.5F, Spark::Vector3{0.0F, 0.5F, 0.0F});
    cc->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Player));
    cc->SetMaskBits(Spark::CollisionFilter3D::AllLayersMask());

    Spark::PhysicsQueryWorld3D queries{};
    queries.RebuildDynamics(world);

    Spark::Array<Spark::PhysicsQueryHitDynamic3D> hits{};
    queries.OverlapSphereDynamics(Spark::Vector3{1.0F, 0.5F, 0.0F}, 0.6F, DefaultSolidQueryFilter(), nullptr, hits);
    EXPECT_EQ(hits.GetSize(), 1u);
    EXPECT_EQ(hits[0].owner, player);
}

TEST(PhysicsWorld3DTest, FixedTimestepRunsDeterministicSteps) {
    Spark::GameWorld world{};
    AddStaticFloor(world, Spark::Vector3{0.0F, -0.5F, 0.0F}, Spark::Vector3{20.0F, 0.5F, 20.0F});
    Spark::GameObject* ball = AddDynamicSphere(
            world,
            Spark::Vector3{0.0F, 5.0F, 0.0F},
            0.25F,
            Spark::Vector3::Zero,
            Spark::CollisionFilter3D::DefaultCategory());

    Spark::PhysicsWorld3D sim{};
    sim.GetSettings().gravityY = -10.0F;
    sim.GetSettings().fixedSimulationTimestep = 1.0F / 60.0F;
    sim.GetSettings().maxFixedSimulationStepsPerFrame = 8;
    sim.GetSettings().substeps = 1;

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.05F;
    sim.Simulate(world, timing);

    const Spark::Vector3 pos = ball->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation;
    EXPECT_LT(pos.y, 5.0F);
}

TEST(PhysicsWorld3DTest, FastBallDoesNotTunnelThroughFloor) {
    Spark::GameWorld world{};
    AddStaticFloor(world, Spark::Vector3{0.0F, 0.0F, 0.0F}, Spark::Vector3{20.0F, 0.5F, 20.0F});
    Spark::GameObject* ball = AddDynamicSphere(
            world,
            Spark::Vector3{0.0F, 1.5F, 0.0F},
            0.2F,
            Spark::Vector3{0.0F, -24.0F, 0.0F},
            Spark::CollisionFilter3D::DefaultCategory());

    Spark::PhysicsWorld3D sim{};
    sim.GetSettings().gravityY = 0.0F;
    sim.GetSettings().sweptStaticCcdBinaryIterations = 12;
    sim.GetSettings().substeps = 1;

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.05F;
    sim.Simulate(world, timing);

    const float y = ball->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation.y;
    EXPECT_GE(y, 0.45F);
}

TEST(PhysicsWorld3DTest, DynamicCcdSlowsBallAgainstMovingObstacle) {
    Spark::GameWorld world{};
    AddDynamicSphere(
            world,
            Spark::Vector3{0.0F, 1.0F, 0.0F},
            0.6F,
            Spark::Vector3::Zero,
            Spark::CollisionFilter3D::DefaultCategory());

    Spark::GameObject* ball = AddDynamicSphere(
            world,
            Spark::Vector3{0.0F, 1.0F, -3.0F},
            0.2F,
            Spark::Vector3{0.0F, 0.0F, 120.0F},
            Spark::CollisionFilter3D::DefaultCategory());

    Spark::PhysicsWorld3D sim{};
    sim.GetSettings().gravityY = 0.0F;
    sim.GetSettings().sweptDynamicCcdBinaryIterations = 10;
    sim.GetSettings().sweptStaticCcdBinaryIterations = 0;
    sim.GetSettings().substeps = 1;

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.02F;
    sim.Simulate(world, timing);

    const Spark::Vector3 ballPos = ball->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation;
    EXPECT_GT(ballPos.z, -2.5F);
}

TEST(PhysicsSubsystemTest, Simulate3DSyncsQueryCellSize) {
    Spark::GameWorld world{};
    Spark::PhysicsSubsystem physics{};
    physics.GetWorld3D().SetBroadPhaseCellSize(3.5F);
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.016F;
    physics.Simulate3D(world, timing);
    EXPECT_FLOAT_EQ(physics.GetQueries3D().GetCellWorldSize(), 3.5F);
}

TEST(PhysicsShowcaseRegressionTest, CharacterControllerWalksOnFloor) {
    Spark::GameWorld world{};
    AddStaticFloor(world, Spark::Vector3{0.0F, 0.0F, 0.0F}, Spark::Vector3{32.0F, 0.18F, 32.0F});

    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 0.55F, -4.0F});
    auto* cc = player->AddComponent<Spark::CharacterController3DComponent>(0.38F, Spark::Vector3{0.0F, 0.92F, 0.0F});
    cc->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Player));
    cc->SetMaskBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Environment));

    Spark::PhysicsSubsystem physics{};
    physics.GetWorld3D().GetSettings().gravityY = -18.0F;
    physics.GetCharacterController3D().GetSettings().gravityY = -18.0F;

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;
    for (int i = 0; i < 240; ++i) {
        cc->SetMoveInput({0.0F, 0.0F, 6.0F});
        physics.SimulateAll3D(world, timing);
        ++timing.frameIndex;
    }

    const float z = player->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation.z;
    EXPECT_GT(z, -2.0F);
}
