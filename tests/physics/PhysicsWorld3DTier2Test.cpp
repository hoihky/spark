#include <gtest/gtest.h>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/physics/3d/BoxCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/PhysicsContactListener3DComponent.hpp"
#include "spark/ecs/components/physics/3d/Rigidbody3DComponent.hpp"
#include "spark/ecs/components/physics/3d/SphereCollider3DComponent.hpp"
#include "spark/ecs/components/physics/3d/CharacterController3DComponent.hpp"
#include "spark/ecs/components/physics/3d/TriggerVolume3DComponent.hpp"
#include "spark/physics/colliders/PhysicsCollider3DFilter.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/physics/CollisionFilter3D.hpp"
#include "spark/physics/PhysicsBodyHandoff3D.hpp"
#include "spark/physics/PhysicsSubsystem.hpp"
#include "spark/physics/simulation/PhysicsSimulationRecorder3D.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/core/Scene.hpp"
#include "spark/scene/query/ScenePhysicsRaycast3D.hpp"

namespace {

Spark::GameObject* AddFloor(Spark::GameWorld& world) {
    Spark::GameObject* floor = world.CreateGameObject();
    floor->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 0.0F, 0.0F});
    auto* box = floor->AddComponent<Spark::BoxCollider3DComponent>(Spark::Vector3{10.0F, 0.5F, 10.0F});
    box->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Environment));
    box->SetMaskBits(Spark::CollisionFilter3D::AllLayersMask());
    return floor;
}

}  // namespace

TEST(TriggerVolume3DTier2Test, StayFiresWhileOverlapping) {
    Spark::GameWorld world{};
    Spark::GameObject* zone = world.CreateGameObject();
    zone->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 0.0F, 0.0F});
    auto* trigger = zone->AddComponent<Spark::TriggerVolume3DComponent>(
            Spark::TriggerVolume3DShape::Box, Spark::Vector3{2.0F, 2.0F, 2.0F});
    trigger->SetCategoryBits(Spark::CollisionFilter3D::AllLayersMask());
    trigger->SetMaskBits(Spark::CollisionFilter3D::AllLayersMask());

    Spark::GameObject* probe = world.CreateGameObject();
    probe->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 0.0F, 0.0F});
    probe->AddComponent<Spark::SphereCollider3DComponent>(0.4F);

    int enterCount = 0;
    int stayCount = 0;
    trigger->SetOnEnter([&](Spark::GameObject&) { ++enterCount; });
    trigger->SetOnStay([&](Spark::GameObject&) { ++stayCount; });

    Spark::PhysicsSubsystem physics{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.016F;
    physics.SimulateTriggerVolumes3D(world, timing);
    physics.SimulateTriggerVolumes3D(world, timing);

    EXPECT_EQ(enterCount, 1);
    EXPECT_EQ(stayCount, 1);
}

TEST(TriggerVolume3DTier2Test, CharacterControllerUsesPlayerLayerForFilters) {
    Spark::GameWorld world{};
    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::TransformComponent>();
    auto* cc = player->AddComponent<Spark::CharacterController3DComponent>(0.4F);
    cc->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Player));
    cc->SetMaskBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Environment));

    const Spark::ColliderFilter filter = Spark::PhysicsCollider3DFilter::Resolve(*player);
    EXPECT_EQ(filter.categoryBits, cc->GetCategoryBits());
    EXPECT_EQ(filter.maskBits, cc->GetMaskBits());
}

TEST(TriggerVolume3DTier2Test, LayerMaskBlocksProbe) {
    Spark::GameWorld world{};
    Spark::GameObject* zone = world.CreateGameObject();
    zone->AddComponent<Spark::TransformComponent>();
    auto* trigger = zone->AddComponent<Spark::TriggerVolume3DComponent>(
            Spark::TriggerVolume3DShape::Sphere, Spark::Vector3{1.0F, 1.0F, 1.0F});
    trigger->SetShape(Spark::TriggerVolume3DShape::Sphere);
    trigger->SetRadius(2.0F);
    trigger->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Trigger));
    trigger->SetMaskBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Player));

    Spark::GameObject* enemy = world.CreateGameObject();
    enemy->AddComponent<Spark::TransformComponent>();
    auto* sphere = enemy->AddComponent<Spark::SphereCollider3DComponent>(0.5F);
    sphere->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Enemy));
    sphere->SetMaskBits(Spark::CollisionFilter3D::AllLayersMask());

    EXPECT_FALSE(trigger->IsOverlapping(*enemy));

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.016F;
    Spark::PhysicsSubsystem physics{};
    physics.SimulateTriggerVolumes3D(world, timing);
    EXPECT_FALSE(trigger->IsOverlapping(*enemy));
}

TEST(PhysicsContactTier2Test, ListenerReceivesFloorContactOncePerDispatch) {
    Spark::GameWorld world{};
    AddFloor(world);

    Spark::GameObject* ball = world.CreateGameObject();
    ball->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 1.0F, 0.0F});
    ball->AddComponent<Spark::SphereCollider3DComponent>(0.35F);
    auto* rb = ball->AddComponent<Spark::Rigidbody3DComponent>();
    rb->SetVelocity(Spark::Vector3{0.0F, -2.0F, 0.0F});

    int contactCount = 0;
    ball->AddComponent<Spark::PhysicsContactListener3DComponent>()->SetContactHandler(
            [&](const Spark::PhysicsContactReport3D& report) {
                if (report.otherIsStatic) {
                    ++contactCount;
                }
            });

    Spark::PhysicsSubsystem physics{};
    physics.GetWorld3D().GetContactBroadcaster().SetEnabled(true);
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.05F;
    physics.Simulate3D(world, timing);
    EXPECT_GE(contactCount, 1);
    EXPECT_LE(contactCount, 8);
}

TEST(PhysicsBodyHandoffTier2Test, AttachedBodyFollowsCarrier) {
    Spark::GameWorld world{};
    Spark::GameObject* carrier = world.CreateGameObject();
    carrier->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{1.0F, 2.0F, 0.0F});

    Spark::GameObject* crate = world.CreateGameObject();
    crate->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 0.0F, 0.0F});
    crate->AddComponent<Spark::SphereCollider3DComponent>(0.3F);
    crate->AddComponent<Spark::Rigidbody3DComponent>();

    Spark::PhysicsBodyHandoff3D::AttachRigidbodyToCarrier(world, *crate, *carrier, Spark::Vector3{0.0F, 0.5F, 0.0F});

    carrier->GetComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{4.0F, 2.0F, 0.0F});
    Spark::PhysicsBodyHandoff3D::SynchronizeAttachedBodies(world);

    const Spark::Vector3 pos = crate->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation;
    EXPECT_NEAR(pos.x, 4.0F, 0.05F);
    EXPECT_NEAR(pos.y, 2.5F, 0.05F);
}

TEST(PhysicsBodyHandoffTier2Test, ReleaseRestoresDynamicMotion) {
    Spark::GameWorld world{};
    Spark::GameObject* carrier = world.CreateGameObject();
    carrier->AddComponent<Spark::TransformComponent>();

    Spark::GameObject* crate = world.CreateGameObject();
    crate->AddComponent<Spark::TransformComponent>();
    crate->AddComponent<Spark::SphereCollider3DComponent>(0.3F);
    auto* rb = crate->AddComponent<Spark::Rigidbody3DComponent>();

    Spark::PhysicsBodyHandoff3D::AttachRigidbodyToCarrier(world, *crate, *carrier, Spark::Vector3::Zero);
    EXPECT_EQ(rb->GetBodyType(), Spark::RigidbodyBodyType3D::Kinematic);

    Spark::PhysicsBodyHandoff3D::ReleaseRigidbody(*crate, Spark::Vector3{0.0F, 5.0F, 0.0F});
    EXPECT_EQ(rb->GetBodyType(), Spark::RigidbodyBodyType3D::Dynamic);
    EXPECT_FLOAT_EQ(rb->GetVelocity().y, 5.0F);
}

TEST(PhysicsBodyHandoffTier2Test, ReleasedBodyTranslatesAfterSimulation) {
    Spark::GameWorld world{};
    Spark::GameObject* floor = world.CreateGameObject();
    floor->AddComponent<Spark::TransformComponent>();
    floor->AddComponent<Spark::BoxCollider3DComponent>(Spark::Vector3{10.0F, 0.5F, 10.0F}, Spark::Vector3{0.0F, 0.5F, 0.0F});
    floor->AddComponent<Spark::Rigidbody3DComponent>(Spark::RigidbodyBodyType3D::Static, 1.0F);

    Spark::GameObject* carrier = world.CreateGameObject();
    carrier->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 2.0F, 0.0F});

    Spark::GameObject* crate = world.CreateGameObject();
    crate->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 2.0F, 0.0F});
    crate->AddComponent<Spark::SphereCollider3DComponent>(0.3F);
    crate->AddComponent<Spark::Rigidbody3DComponent>();

    Spark::PhysicsBodyHandoff3D::AttachRigidbodyToCarrier(world, *crate, *carrier, Spark::Vector3::Zero);
    Spark::PhysicsBodyHandoff3D::ReleaseRigidbody(*crate, Spark::Vector3{0.0F, 0.0F, 8.0F});

    const float startZ = crate->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation.z;

    Spark::PhysicsSubsystem physics{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 1.0F / 60.0F;
    for (int i = 0; i < 30; ++i) {
        physics.Simulate3D(world, timing);
    }

    const float endZ = crate->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation.z;
    EXPECT_GT(endZ, startZ + 0.5F);
}

TEST(PhysicsSimulationDomainTier2Test, PausedSimulationDoesNotIntegrate) {
    Spark::GameWorld world{};
    AddFloor(world);

    Spark::GameObject* ball = world.CreateGameObject();
    ball->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 3.0F, 0.0F});
    ball->AddComponent<Spark::SphereCollider3DComponent>(0.25F);
    auto* rb = ball->AddComponent<Spark::Rigidbody3DComponent>();
    rb->SetVelocity(Spark::Vector3{0.0F, -10.0F, 0.0F});

    Spark::PhysicsSubsystem physics{};
    physics.GetSimulationDomain3D().SetAdvanceMode(Spark::PhysicsSimulationDomain3D::AdvanceMode::Paused);

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.1F;
    physics.Simulate3D(world, timing);

    EXPECT_FLOAT_EQ(ball->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation.y, 3.0F);
}

TEST(PhysicsSimulationDomainTier2Test, ReplayBufferCapturesAndRestores) {
    Spark::GameWorld world{};
    Spark::GameObject* ball = world.CreateGameObject();
    ball->AddComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{0.0F, 2.0F, 0.0F});
    ball->AddComponent<Spark::SphereCollider3DComponent>(0.3F);
    auto* rb = ball->AddComponent<Spark::Rigidbody3DComponent>();
    rb->SetVelocity(Spark::Vector3{1.0F, 0.0F, 0.0F});

    Spark::PhysicsSubsystem physics{};
    physics.GetWorld3D().GetSettings().gravityY = 0.0F;
    auto& domain = physics.GetSimulationDomain3D();
    domain.SetRecordingEnabled(true);
    domain.GetReplayBuffer().SetCapacity(8);

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.05F;
    physics.Simulate3D(world, timing);
    const float recordedX = ball->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation.x;
    ball->GetComponent<Spark::TransformComponent>()->SetTranslation(Spark::Vector3{9.0F, 2.0F, 0.0F});

    EXPECT_TRUE(physics.PlaybackPhysics3DFrame(world, 0));
    EXPECT_NEAR(ball->GetComponent<Spark::TransformComponent>()->GetLocalTransform().translation.x, recordedX, 0.05F);
}

TEST(ScenePhysicsRaycastTier2Test, PhysicsHitWinsWhenCloserThanMesh) {
    Spark::Scene scene{};
    Spark::GameWorld& world = scene.GetWorld();

    AddFloor(world);

    Spark::PhysicsSubsystem physics{};
    Spark::ScenePhysicsRaycast3D unified{scene};
    unified.SetPhysicsQueries(&physics.GetQueries3D());

    Spark::SceneRay ray{};
    ray.origin = Spark::Vector3{0.0F, 5.0F, 0.0F};
    ray.direction = Spark::Vector3{0.0F, -1.0F, 0.0F};
    ray.tMax = 20.0F;

    Spark::SceneRaycastHit hit{};
    Spark::SceneRaycastOptions sceneOpts{};
    sceneOpts.pickRigidMeshes = false;
    sceneOpts.pickSkinnedMeshes = false;

    Spark::PhysicsQueryFilter3D filter{};
    filter.queryCategoryBits = Spark::CollisionFilter3D::DefaultCategory();
    filter.queryMaskBits = Spark::CollisionFilter3D::AllLayersMask();

    const bool ok = unified.RaycastClosest(world, ray, hit, sceneOpts, filter, nullptr);
    EXPECT_TRUE(ok);
    EXPECT_NE(hit.object, nullptr);
    EXPECT_LT(hit.distance, 5.0F);
}
