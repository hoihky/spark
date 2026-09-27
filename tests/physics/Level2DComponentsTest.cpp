#include <gtest/gtest.h>

#include <cmath>
#include <exception>

#include "spark/ecs/components/camera/Camera2DRigComponent.hpp"
#include "spark/ecs/components/camera/CameraBounds2DComponent.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/gameplay/DamageZone2DComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/physics/2d/Hurtbox2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/ecs/components/world/SpawnPoint2DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/gameplay/CameraBounds2DResolve.hpp"
#include "spark/math/Quaternion.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"
#include "spark/scene/core/SceneSpawn2D.hpp"
#include "spark/scene/core/GameWorld.hpp"

namespace {

class NoOpEngineContext final : public Spark::IEngineContext {
public:
    Spark::Window& GetWindow() override { std::terminate(); }
    Spark::IFramePresenter& GetFramePresenter() override { std::terminate(); }
    Spark::IInput& GetInput() override { std::terminate(); }
    void GetFramebufferSize(int& outWidth, int& outHeight) const override {
        outWidth = 1280;
        outHeight = 720;
    }
    void SetSceneRenderParams(const Spark::SceneRenderParams& /*params*/) override {}
    Spark::SoundEngine* TryGetSoundEngine() noexcept override { return nullptr; }
    Spark::Scene* TryGetScene() noexcept override { return nullptr; }
    Spark::IImGuiLayer* TryGetImGuiLayer() noexcept override { return nullptr; }
};

}  // namespace

TEST(Level2DComponents, SpawnPoint2DResolvesFacingFromTransform) {
    Spark::GameWorld world{};
    Spark::GameObject* spawn = world.CreateGameObject();
    auto* tr = spawn->AddComponent<Spark::TransformComponent>();
    tr->SetRotation(Spark::Quaternion::FromAxisAngle(Spark::Vector3::UnitZ, 1.5707963F));
    auto* marker = spawn->AddComponent<Spark::SpawnPoint2DComponent>();
    marker->SetSpawnName("Checkpoint");
    marker->SetUseTransformFacing(true);

    const Spark::SceneSpawnPose2D pose = Spark::FindSpawnPoint2D(world, "Checkpoint");
    EXPECT_TRUE(pose.found);
    EXPECT_NEAR(pose.facingRadians, 1.5707963F, 0.001F);
}

TEST(Level2DComponents, CameraBounds2DSelectsContainingVolume) {
    Spark::GameWorld world{};
    Spark::GameObject* player = world.CreateGameObject();
    player->AddComponent<Spark::TransformComponent>()->SetTranslation({4.0F, 2.0F, 0.0F});

    Spark::GameObject* zone = world.CreateGameObject();
    zone->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 0.0F, 0.0F});
    auto* bounds = zone->AddComponent<Spark::CameraBounds2DComponent>();
    bounds->SetHalfExtents({8.0F, 6.0F});
    bounds->SetPriority(2);

    Spark::Vector2 minOut{};
    Spark::Vector2 maxOut{};
    EXPECT_TRUE(Spark::TryResolveCameraBounds2DForTarget(world, *player, minOut, maxOut));
    EXPECT_NEAR(minOut.x, -8.0F, 0.001F);
    EXPECT_NEAR(maxOut.x, 8.0F, 0.001F);
    EXPECT_NEAR(minOut.y, -6.0F, 0.001F);
    EXPECT_NEAR(maxOut.y, 6.0F, 0.001F);
}

TEST(Level2DComponents, DamageZone2DAppliesContinuousDamage) {
    Spark::GameWorld world{};
    constexpr std::uint16_t kHurtLayer = 1u << 6;

    Spark::GameObject* victim = world.CreateGameObject();
    victim->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 0.0F, 0.0F});
    victim->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Static, 0.0F);
    auto* hurtbox = victim->AddComponent<Spark::Hurtbox2DComponent>();
    hurtbox->SetRadius(0.5F);
    hurtbox->SetCategoryBits(kHurtLayer);
    auto* health = victim->AddComponent<Spark::HealthComponent>(10.0F);

    Spark::GameObject* hazard = world.CreateGameObject();
    hazard->AddComponent<Spark::TransformComponent>();
    auto* zone = hazard->AddComponent<Spark::DamageZone2DComponent>();
    zone->SetHalfExtents({3.0F, 3.0F});
    zone->SetDamagePerSecond(20.0F);
    Spark::PhysicsQueryFilter2D filter{};
    filter.queryCategoryBits = 1u << 2;
    filter.queryMaskBits = kHurtLayer;
    filter.hitSolids = false;
    filter.hitTriggers = true;
    zone->SetTargetFilter(filter);

    NoOpEngineContext context{};
    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.5F;
    zone->OnUpdate(timing, *hazard, context);

    EXPECT_LT(health->GetCurrent(), 10.0F);
}
