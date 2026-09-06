#include <gtest/gtest.h>

#include "spark/core/HashMap.hpp"
#include "spark/ecs/components/ai/AiAgentComponent.hpp"
#include "spark/ecs/components/ai/PerceptionSensorComponent.hpp"
#include "spark/ecs/components/animation/AnimationEventReceiverComponent.hpp"
#include "spark/ecs/components/animation/AttachmentSocketComponent.hpp"
#include "spark/ecs/components/animation/Character3DAnimFsmComponent.hpp"
#include "spark/ecs/components/physics/2d/BoxCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/DistanceJoint2DComponent.hpp"
#include "spark/ecs/components/physics/2d/HingeJoint2DComponent.hpp"
#include "spark/ecs/components/physics/2d/PhysicsMaterial2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/physics/2d/TilemapCollider2DComponent.hpp"
#include "spark/ecs/components/rendering/TilemapComponent.hpp"
#include "spark/ecs/components/tilemap/TilemapGameplayGridComponent.hpp"
#include "spark/ecs/components/ui/UiCanvasComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"
#include "spark/scene/texture/Texture2D.hpp"
#include "spark/scene/tilemap/Tileset.hpp"
#include "spark/ui/spark/controls/SparkControls.hpp"
#include "spark/ui/spark/SparkUiControlsFactory.hpp"

namespace {

void RoundTripComponent(
        Spark::GameObject& source,
        Spark::GameObject& restored,
        const Spark::ComponentKind kind) {
    const Spark::IComponentSnapshotHandler* handler = Spark::ComponentSnapshotRegistry::Default().Find(kind);
    ASSERT_NE(handler, nullptr);
    Spark::SceneCaptureContext captureCtx{};
    Spark::ComponentRecord captured{};
    ASSERT_TRUE(handler->TryCapture(source, captureCtx, captured));

    Spark::GameWorld restoreWorld{};
    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = "assets";
    applyCtx.assetLoader = &restoreWorld.GetAssetLoader();
    ASSERT_TRUE(handler->TryRestore(restored, captured, restoreWorld, applyCtx));
}

}  // namespace

TEST(GameplayComponentRoundTripTest, Rigidbody2DAndCollidersRoundTrip) {
    Spark::GameWorld world{};
    Spark::GameObject* source = world.CreateGameObject();
    Spark::GameObject* restored = world.CreateGameObject();

    Spark::Rigidbody2DComponent* rb = source->AddComponent<Spark::Rigidbody2DComponent>();
    rb->SetBodyType(Spark::RigidbodyBodyType2D::Dynamic);
    rb->SetGravityScale(0.5F);
    rb->SetVelocity({1.2F, -0.4F});

    Spark::BoxCollider2DComponent* box = source->AddComponent<Spark::BoxCollider2DComponent>();
    box->SetHalfExtents({0.8F, 0.4F});
    box->SetOffset({0.1F, 0.2F});
    box->SetCategoryBits(0x2);
    box->SetMaskBits(0x4);
    box->SetIsTrigger(true);

    RoundTripComponent(*source, *restored, Spark::ComponentKind::Rigidbody2D);
    RoundTripComponent(*source, *restored, Spark::ComponentKind::BoxCollider2D);

    const Spark::Rigidbody2DComponent* restoredRb = restored->GetComponent<Spark::Rigidbody2DComponent>();
    ASSERT_NE(restoredRb, nullptr);
    EXPECT_EQ(restoredRb->GetBodyType(), Spark::RigidbodyBodyType2D::Dynamic);
    EXPECT_FLOAT_EQ(restoredRb->GetGravityScale(), 0.5F);
    EXPECT_FLOAT_EQ(restoredRb->GetVelocity().x, 1.2F);

    const Spark::BoxCollider2DComponent* restoredBox = restored->GetComponent<Spark::BoxCollider2DComponent>();
    ASSERT_NE(restoredBox, nullptr);
    EXPECT_FLOAT_EQ(restoredBox->GetHalfExtents().x, 0.8F);
    EXPECT_EQ(restoredBox->GetCategoryBits(), 0x2);
    EXPECT_TRUE(restoredBox->GetIsTrigger());
}

TEST(GameplayComponentRoundTripTest, Physics2DExtendedRoundTrip) {
    Spark::GameWorld world{};
    Spark::GameObject* bodyA = world.CreateGameObject();
    Spark::GameObject* bodyB = world.CreateGameObject();
    Spark::GameObject* restored = world.CreateGameObject();

    Spark::PhysicsMaterial2DComponent* material = bodyA->AddComponent<Spark::PhysicsMaterial2DComponent>();
    material->SetDynamicFriction(0.25F);
    material->SetRestitution(0.75F);

    Spark::DistanceJoint2DComponent* distance = bodyA->AddComponent<Spark::DistanceJoint2DComponent>();
    distance->SetConnectedBody(bodyB);
    distance->SetRestLength(3.5F);
    distance->SetLocalAnchorA({0.2F, 0.0F});
    distance->SetLocalAnchorB({-0.3F, 0.1F});
    distance->SetStiffness(0.9F);

    Spark::HingeJoint2DComponent* hinge = bodyB->AddComponent<Spark::HingeJoint2DComponent>();
    hinge->SetConnectedBody(bodyA);
    hinge->SetLocalAnchorA({0.0F, 1.0F});
    hinge->SetLocalAnchorB({0.0F, -1.0F});
    hinge->SetStiffness(0.4F);

    RoundTripComponent(*bodyA, *restored, Spark::ComponentKind::PhysicsMaterial2D);

    Spark::HashMap<std::uint64_t, Spark::GameObject*> lookup{};
    lookup.Add(bodyA->GetId(), bodyA);
    lookup.Add(bodyB->GetId(), bodyB);
    Spark::GameObject* restoredDistance = world.CreateGameObject();
    Spark::GameObject* restoredHinge = world.CreateGameObject();

    const Spark::IComponentSnapshotHandler* distanceHandler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::DistanceJoint2D);
    const Spark::IComponentSnapshotHandler* hingeHandler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::HingeJoint2D);
    ASSERT_NE(distanceHandler, nullptr);
    ASSERT_NE(hingeHandler, nullptr);

    Spark::ComponentRecord distanceRecord{};
    Spark::ComponentRecord hingeRecord{};
    ASSERT_TRUE(distanceHandler->TryCapture(*bodyA, {}, distanceRecord));
    ASSERT_TRUE(hingeHandler->TryCapture(*bodyB, {}, hingeRecord));

    Spark::SceneApplyContext applyCtx{};
    applyCtx.entityLookup = &lookup;
    ASSERT_TRUE(distanceHandler->TryRestore(*restoredDistance, distanceRecord, world, applyCtx));
    ASSERT_TRUE(hingeHandler->TryRestore(*restoredHinge, hingeRecord, world, applyCtx));

    const Spark::PhysicsMaterial2DComponent* restoredMaterial = restored->GetComponent<Spark::PhysicsMaterial2DComponent>();
    ASSERT_NE(restoredMaterial, nullptr);
    EXPECT_FLOAT_EQ(restoredMaterial->GetDynamicFriction(), 0.25F);

    const Spark::DistanceJoint2DComponent* restoredDistanceJoint =
            restoredDistance->GetComponent<Spark::DistanceJoint2DComponent>();
    ASSERT_NE(restoredDistanceJoint, nullptr);
    EXPECT_EQ(restoredDistanceJoint->GetConnectedBody(), bodyB);
    EXPECT_FLOAT_EQ(restoredDistanceJoint->GetRestLength(), 3.5F);

    const Spark::HingeJoint2DComponent* restoredHingeJoint = restoredHinge->GetComponent<Spark::HingeJoint2DComponent>();
    ASSERT_NE(restoredHingeJoint, nullptr);
    EXPECT_EQ(restoredHingeJoint->GetConnectedBody(), bodyA);
}

TEST(GameplayComponentRoundTripTest, TilemapStackRoundTrip) {
    Spark::GameWorld world{};
    Spark::GameObject* source = world.CreateGameObject();
    Spark::GameObject* restored = world.CreateGameObject();

    Spark::Texture2D placeholder{};
    placeholder.GetName() = Spark::Utf8String("tests/tilemap_atlas");
    const Spark::SharedPtr<Spark::Texture2D> atlas =
            world.RegisterTexture(Spark::MakeShared<Spark::Texture2D>(Spark::MoveTemp(placeholder)), "tests/tilemap_atlas");

    Spark::TilemapComponent* tilemap = source->AddComponent<Spark::TilemapComponent>(
            Spark::MakeShared<Spark::Tileset>(atlas, 8U, 8U), 4U, 4U, 1.0F, 0);
    tilemap->SetTile(0U, 1U, 2U, 3U);
    tilemap->SetTile(0U, 2U, 3U, 5U);
    source->AddComponent<Spark::TilemapCollider2DComponent>()->SetCategoryBits(0x8);
    Spark::TilemapGameplayGridComponent* grid = source->AddComponent<Spark::TilemapGameplayGridComponent>();
    grid->SetWalkRule(Spark::TilemapGameplayWalkRule::OccupiedWalkable);
    grid->SetAutoRebake(true);

    RoundTripComponent(*source, *restored, Spark::ComponentKind::Tilemap);
    RoundTripComponent(*source, *restored, Spark::ComponentKind::TilemapCollider2D);
    RoundTripComponent(*source, *restored, Spark::ComponentKind::TilemapGameplayGrid);

    const Spark::TilemapComponent* restoredTilemap = restored->GetComponent<Spark::TilemapComponent>();
    ASSERT_NE(restoredTilemap, nullptr);
    EXPECT_EQ(restoredTilemap->GetMapWidth(), 4U);
    EXPECT_EQ(restoredTilemap->GetTileCell(0U, 1U, 2U).tileId, 3U);
    EXPECT_EQ(restoredTilemap->GetTileCell(0U, 2U, 3U).tileId, 5U);

    const Spark::TilemapCollider2DComponent* restoredCollider = restored->GetComponent<Spark::TilemapCollider2DComponent>();
    ASSERT_NE(restoredCollider, nullptr);
    EXPECT_EQ(restoredCollider->GetCategoryBits(), 0x8);

    const Spark::TilemapGameplayGridComponent* restoredGrid = restored->GetComponent<Spark::TilemapGameplayGridComponent>();
    ASSERT_NE(restoredGrid, nullptr);
    EXPECT_EQ(restoredGrid->GetWalkRule(), Spark::TilemapGameplayWalkRule::OccupiedWalkable);
    EXPECT_TRUE(restoredGrid->GetAutoRebake());
}

TEST(GameplayComponentRoundTripTest, UiCanvasWidgetTreeRoundTrip) {
    Spark::GameWorld world{};
    Spark::GameObject* source = world.CreateGameObject();
    Spark::GameObject* restored = world.CreateGameObject();

    Spark::UiCanvasComponent* canvas = source->AddComponent<Spark::UiCanvasComponent>();
    canvas->SetSortOrder(12);
    canvas->SetModalInputCapture(true);
    Spark::Ui::SparkUiControlsFactory factory{};
    Spark::Ui::PanelDesc panelDesc{};
    panelDesc.id = Spark::Utf8String("root_panel");
    panelDesc.title = Spark::Utf8String("HUD");
    panelDesc.width = 320.0F;
    panelDesc.height = 180.0F;
    Spark::UniquePtr<Spark::Ui::IPanel> panel = factory.CreatePanel(panelDesc);
    Spark::Ui::ButtonDesc buttonDesc{};
    buttonDesc.id = Spark::Utf8String("play_btn");
    buttonDesc.label = Spark::Utf8String("Play");
    panel->AddChild(Spark::UniquePtr<Spark::Ui::IUiElement>(
            static_cast<Spark::Ui::IUiElement*>(factory.CreateButton(buttonDesc).Release())));
    canvas->AdoptRoot(Spark::UniquePtr<Spark::Ui::IUiElement>(
            static_cast<Spark::Ui::IUiElement*>(panel.Release())));

    RoundTripComponent(*source, *restored, Spark::ComponentKind::UiCanvas);

    const Spark::UiCanvasComponent* restoredCanvas = restored->GetComponent<Spark::UiCanvasComponent>();
    ASSERT_NE(restoredCanvas, nullptr);
    EXPECT_EQ(restoredCanvas->GetSortOrder(), 12);
    EXPECT_TRUE(restoredCanvas->GetModalInputCapture());
    ASSERT_NE(restoredCanvas->GetRoot(), nullptr);
    const Spark::Ui::SparkPanel* restoredPanel =
            dynamic_cast<const Spark::Ui::SparkPanel*>(restoredCanvas->GetRoot());
    ASSERT_NE(restoredPanel, nullptr);
    EXPECT_STREQ(restoredPanel->ExportDesc().title.CStr(), "HUD");
    ASSERT_EQ(restoredPanel->GetChildren().GetSize(), 1U);
    const Spark::Ui::IButton* restoredButton =
            dynamic_cast<const Spark::Ui::IButton*>(restoredPanel->GetChildren()[0].Get());
    ASSERT_NE(restoredButton, nullptr);
    EXPECT_STREQ(restoredButton->GetLabel().CStr(), "Play");
}

TEST(GameplayComponentRoundTripTest, AiAndAnimationRoundTrip) {
    Spark::GameWorld world{};
    Spark::GameObject* source = world.CreateGameObject();
    Spark::GameObject* restored = world.CreateGameObject();

    Spark::AiAgentComponent* agent = source->AddComponent<Spark::AiAgentComponent>();
    agent->SetEnabled(true);
    agent->SetMaxSpeed(6.5F);
    agent->SetSteeringPlane(Spark::AiSteeringPlane::XyRigidbody2D);
    agent->SetGoapEnabled(true);
    agent->SetGoapWorldBits(0xABCDULL);
    agent->SetGoapGoal(0x0F, 0x05);
    Spark::GoapActionSpec action{};
    action.preMask = 0x1;
    action.preValue = 0x1;
    action.effectSetMask = 0x2;
    action.cost = 2.5F;
    action.nameId = 7;
    agent->GetGoapActions().PushBack(action);

    Spark::PerceptionSensorComponent* sensor = source->AddComponent<Spark::PerceptionSensorComponent>();
    sensor->SetSightRadius(18.0F);
    sensor->SetHearingRadius(9.0F);
    sensor->SetSightFovDegrees(95.0F);
    sensor->SetTargetCategoryMask(0x3);

    Spark::Character3DAnimFsmComponent* fsm = source->AddComponent<Spark::Character3DAnimFsmComponent>();
    fsm->SetLocomotionClips(0, 1, 2);
    fsm->SetCombatClips(4, 5, 6, 7);
    fsm->SetWalkSpeedThreshold(0.5F);
    fsm->SetRunSpeedThreshold(2.8F);
    fsm->SetCrossfadeDuration(0.25F);
    fsm->SetLocomotionBlendEnabled(true);
    fsm->SetCombatBlackboardIntSlot(13);
    fsm->SetManualClip(3, Spark::AnimLoopMode::Once);

    Spark::AttachmentSocketComponent* socket = source->AddComponent<Spark::AttachmentSocketComponent>();
    socket->SetJointIndex(2);
    socket->SetLocalOffset({0.0F, 0.15F, 0.2F});
    socket->SetEnabled(false);

    Spark::AnimationEventReceiverComponent* events = source->AddComponent<Spark::AnimationEventReceiverComponent>();
    events->AddMarker(1, 0.5F, "footstep");

    RoundTripComponent(*source, *restored, Spark::ComponentKind::AiAgent);
    RoundTripComponent(*source, *restored, Spark::ComponentKind::PerceptionSensor);
    RoundTripComponent(*source, *restored, Spark::ComponentKind::Character3DAnimFsm);
    RoundTripComponent(*source, *restored, Spark::ComponentKind::AttachmentSocket);
    RoundTripComponent(*source, *restored, Spark::ComponentKind::AnimationEventReceiver);

    const Spark::AiAgentComponent* restoredAgent = restored->GetComponent<Spark::AiAgentComponent>();
    ASSERT_NE(restoredAgent, nullptr);
    EXPECT_FLOAT_EQ(restoredAgent->GetMaxSpeed(), 6.5F);
    EXPECT_EQ(restoredAgent->GetGoapActions().GetSize(), 1U);
    EXPECT_FLOAT_EQ(restoredAgent->GetGoapActions()[0].cost, 2.5F);

    const Spark::PerceptionSensorComponent* restoredSensor = restored->GetComponent<Spark::PerceptionSensorComponent>();
    ASSERT_NE(restoredSensor, nullptr);
    EXPECT_FLOAT_EQ(restoredSensor->GetSightRadius(), 18.0F);
    EXPECT_EQ(restoredSensor->GetTargetCategoryMask(), 0x3);

    const Spark::Character3DAnimFsmComponent* restoredFsm = restored->GetComponent<Spark::Character3DAnimFsmComponent>();
    ASSERT_NE(restoredFsm, nullptr);
    EXPECT_EQ(restoredFsm->GetRunClipIndex(), 2U);
    EXPECT_EQ(restoredFsm->GetAttackClipIndex(), 4U);
    EXPECT_EQ(restoredFsm->GetHurtClipIndex(), 5U);
    EXPECT_EQ(restoredFsm->GetStaggerClipIndex(), 6U);
    EXPECT_EQ(restoredFsm->GetDeathClipIndex(), 7U);
    EXPECT_TRUE(restoredFsm->IsLocomotionBlendEnabled());
    EXPECT_EQ(restoredFsm->GetCombatBlackboardIntSlot(), 13U);
    EXPECT_TRUE(restoredFsm->IsManualClipActive());
    EXPECT_EQ(restoredFsm->GetManualClipIndex(), 3U);

    const Spark::AttachmentSocketComponent* restoredSocket = restored->GetComponent<Spark::AttachmentSocketComponent>();
    ASSERT_NE(restoredSocket, nullptr);
    EXPECT_EQ(restoredSocket->GetJointIndex(), 2U);
    EXPECT_FALSE(restoredSocket->IsEnabled());

    const Spark::AnimationEventReceiverComponent* restoredEvents =
            restored->GetComponent<Spark::AnimationEventReceiverComponent>();
    ASSERT_NE(restoredEvents, nullptr);
    ASSERT_EQ(restoredEvents->GetMarkers().GetSize(), 1U);
    EXPECT_STREQ(restoredEvents->GetMarkers()[0].eventName.CStr(), "footstep");
}

TEST(GameplayComponentRoundTripTest, SceneSerializerCapturesGameplayComponents) {
    Spark::GameWorld world{};
    Spark::GameObject* object = world.CreateGameObject();
    object->AddComponent<Spark::Rigidbody2DComponent>();
    object->AddComponent<Spark::PerceptionSensorComponent>();

    Spark::SceneSerializer serializer{};
    Spark::SceneCaptureContext captureCtx{};
    Spark::SceneDocument document = serializer.Capture(world, captureCtx, {});
    bool foundRigidbody = false;
    bool foundPerception = false;
    for (std::size_t ei = 0; ei < document.entities.GetSize(); ++ei) {
        for (std::size_t ci = 0; ci < document.entities[ei].components.GetSize(); ++ci) {
            if (document.entities[ei].components[ci].kind == Spark::Utf8String("rigidbody_2d")) {
                foundRigidbody = true;
            }
            if (document.entities[ei].components[ci].kind == Spark::Utf8String("perception_sensor")) {
                foundPerception = true;
            }
        }
    }
    EXPECT_TRUE(foundRigidbody);
    EXPECT_TRUE(foundPerception);
}
