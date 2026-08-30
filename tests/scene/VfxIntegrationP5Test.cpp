#include <gtest/gtest.h>

#include "spark/ecs/components/animation/AnimationEventReceiverComponent.hpp"
#include "spark/ecs/components/animation/AnimationEventVfxComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/Signal.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/vfx/VfxSubsystemProcess.hpp"

namespace {

TEST(VfxIntegrationP5Test, AnimationEventQueuesVfxPlayRequest) {
    Spark::GameWorld world{};
    Spark::GameObject* actor = world.CreateGameObject();
    actor->AddComponent<Spark::TransformComponent>()->SetTranslation({2.0F, 3.0F, 0.5F});

    Spark::AnimationEventReceiverComponent* receiver = actor->AddComponent<Spark::AnimationEventReceiverComponent>();
    Spark::AnimationEventVfxComponent* vfx = actor->AddComponent<Spark::AnimationEventVfxComponent>();
    vfx->AddBinding("footstep", "explosion");

    Spark::SignalPayload payload{};
    payload.ptr = "footstep";
    actor->EmitSignal(Spark::SignalId::AnimationEvent, payload, receiver);

    Spark::ProcessVfx(world);

    Spark::Array<Spark::SceneParticleInstance> instances{};
    world.ForEachActiveGameObject([&instances](Spark::GameObject* object) {
        if (object == nullptr || object->GetName() != Spark::Utf8String("__vfx_pooled")) {
            return;
        }
        if (const Spark::ParticleEmitterComponent* pe = object->GetComponent<Spark::ParticleEmitterComponent>()) {
            pe->CollectInstances(instances);
        }
    });
    EXPECT_GT(instances.GetSize(), 0u);
}

TEST(VfxIntegrationP5Test, AnimationEventVfxSnapshotRoundTrip) {
    Spark::GameWorld world{};
    Spark::GameObject* source = world.CreateGameObject();
    Spark::GameObject* restored = world.CreateGameObject();
    Spark::AnimationEventVfxComponent* vfx = source->AddComponent<Spark::AnimationEventVfxComponent>();
    vfx->AddBinding("land", "impact");
    vfx->SetWorldOffset({0.0F, 0.1F, 0.0F});

    const Spark::IComponentSnapshotHandler* handler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::AnimationEventVfx);
    ASSERT_NE(handler, nullptr);

    Spark::ComponentRecord captured{};
    Spark::SceneCaptureContext captureCtx{};
    ASSERT_TRUE(handler->TryCapture(*source, captureCtx, captured));

    Spark::SceneApplyContext applyCtx{};
    ASSERT_TRUE(handler->TryRestore(*restored, captured, world, applyCtx));

    const Spark::AnimationEventVfxComponent* restoredVfx = restored->GetComponent<Spark::AnimationEventVfxComponent>();
    ASSERT_NE(restoredVfx, nullptr);
    ASSERT_EQ(restoredVfx->GetBindings().GetSize(), 1u);
    EXPECT_STREQ(restoredVfx->GetBindings()[0].eventName.CStr(), "land");
    EXPECT_STREQ(restoredVfx->GetBindings()[0].vfxAssetKey.CStr(), "impact");
}

}  // namespace
