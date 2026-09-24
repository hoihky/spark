#include <gtest/gtest.h>

#include <GLFW/glfw3.h>

#include "spark/ecs/components/animation/SpriteAnimationEventReceiverComponent.hpp"
#include "spark/ecs/components/camera/ScreenShakeComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/gameplay/GameStateComponent.hpp"
#include "spark/ecs/components/input/InputActionMapComponent.hpp"
#include "spark/ecs/components/rendering/ParallaxLayerComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"

TEST(GameplayFlowComponents, ParallaxLayerFollowsCamera) {
    Spark::GameWorld world{};
    Spark::GameObject* camera = world.CreateGameObject();
    Spark::TransformComponent* cameraTr = camera->AddComponent<Spark::TransformComponent>();
    cameraTr->SetTranslation({10.0F, 0.0F, 0.0F});

    Spark::GameObject* sky = world.CreateGameObject();
    Spark::TransformComponent* skyTr = sky->AddComponent<Spark::TransformComponent>();
    skyTr->SetTranslation({0.0F, 4.0F, -0.4F});
    auto* parallax = sky->AddComponent<Spark::ParallaxLayerComponent>();
    parallax->SetCameraReference(camera);
    parallax->SetAnchorWorld({0.0F, 0.0F, 0.0F});
    parallax->SetFactorX(0.25F);

    Spark::ParallaxLayerComponent::Tick(*parallax, *sky, 0.0F, 0.0F);
    const Spark::Vector3 pos = skyTr->GetLocalTransform().translation;
    EXPECT_NEAR(pos.x, 2.5F, 1.0e-3F);
}

TEST(GameplayFlowComponents, ScreenShakeImpulseDecays) {
    Spark::ScreenShakeComponent shake{};
    shake.AddImpulse({0.4F, 0.2F}, 0.25F, 30.0F);
    shake.Tick(0.05F);
    EXPECT_GT(shake.GetOffset().LengthSquared(), 0.0F);
    shake.Tick(0.30F);
    EXPECT_LT(shake.GetOffset().LengthSquared(), 1.0e-4F);
}

TEST(GameplayFlowComponents, GameStateTransitionInvokesCallback) {
    Spark::GameWorld world{};
    Spark::GameObject* manager = world.CreateGameObject();
    auto* state = manager->AddComponent<Spark::GameStateComponent>(Spark::GameFlowState::Playing);
    bool transitioned = false;
    state->SetOnTransition([&](Spark::GameFlowState, Spark::GameFlowState next, Spark::GameObject&) {
        if (next == Spark::GameFlowState::Victory) {
            transitioned = true;
        }
    });
    EXPECT_TRUE(state->RequestState(Spark::GameFlowState::Victory));
    EXPECT_TRUE(transitioned);
    EXPECT_TRUE(state->IsState(Spark::GameFlowState::Victory));
}

TEST(GameplayFlowComponents, GameStatePushPopStack) {
    Spark::GameWorld world{};
    Spark::GameObject* manager = world.CreateGameObject();
    auto* state = manager->AddComponent<Spark::GameStateComponent>(Spark::GameFlowState::Playing);
    state->PushState(Spark::GameFlowState::Paused);
    EXPECT_TRUE(state->IsState(Spark::GameFlowState::Paused));
    EXPECT_TRUE(state->PopState());
    EXPECT_TRUE(state->IsState(Spark::GameFlowState::Playing));
}

TEST(GameplayFlowComponents, InputActionMapFindsBindings) {
    Spark::InputActionMapComponent map{};
    map.BindAxis1D("MoveX", GLFW_KEY_A, GLFW_KEY_D, GLFW_KEY_LEFT, GLFW_KEY_RIGHT);
    map.BindButton("Jump", GLFW_KEY_SPACE, GLFW_KEY_W);
    EXPECT_NE(map.FindAction("MoveX"), nullptr);
    EXPECT_NE(map.FindAction("Jump"), nullptr);
    EXPECT_EQ(map.FindAction("Missing"), nullptr);
}

TEST(GameplayFlowComponents, SpriteAnimationEventReceiverStoresMarkers) {
    Spark::SpriteAnimationEventReceiverComponent receiver{};
    receiver.AddMarker(1, 0.5F, "footstep");
    receiver.AddMarker(2, 0.25F, "swing");
    EXPECT_EQ(receiver.GetMarkers().GetSize(), 2U);
    EXPECT_STREQ(receiver.GetMarkers()[0].eventName.CStr(), "footstep");
}
