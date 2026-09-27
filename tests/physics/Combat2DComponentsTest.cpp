#include <gtest/gtest.h>

#include <exception>

#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/physics/2d/CircleCollider2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Hurtbox2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Projectile2DComponent.hpp"
#include "spark/ecs/components/physics/2d/Rigidbody2DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/gameplay/CombatDamage2D.hpp"
#include "spark/physics/PhysicsQueries2D.hpp"
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

NoOpEngineContext gTestEngineContext{};

}  // namespace

TEST(Combat2DComponents, HurtboxInvulnerabilityBlocksDamage) {
    Spark::GameWorld world{};
    Spark::GameObject* victim = world.CreateGameObject();
    victim->AddComponent<Spark::HealthComponent>(10.0F);
    auto* hurtbox = victim->AddComponent<Spark::Hurtbox2DComponent>();
    hurtbox->SetInvulnerabilitySeconds(0.5F);
    hurtbox->NotifyDamageReceived();

    EXPECT_TRUE(hurtbox->IsInvulnerable());
    const float blocked = Spark::TryApplyCombatDamage2D(*victim, nullptr, 3.0F);
    EXPECT_FLOAT_EQ(blocked, 0.0F);

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.6F;
    hurtbox->OnUpdate(timing, *victim, gTestEngineContext);

    EXPECT_FALSE(hurtbox->IsInvulnerable());
    const float applied = Spark::TryApplyCombatDamage2D(*victim, nullptr, 3.0F);
    EXPECT_GT(applied, 0.0F);
}

TEST(Combat2DComponents, HurtboxSyncsTriggerCollider) {
    Spark::GameWorld world{};
    Spark::GameObject* target = world.CreateGameObject();
    auto* hurtbox = target->AddComponent<Spark::Hurtbox2DComponent>();
    hurtbox->SetRadius(0.42F);
    hurtbox->SetCategoryBits(1u << 4);

    Spark::CircleCollider2DComponent* circle = target->GetComponent<Spark::CircleCollider2DComponent>();
    ASSERT_NE(circle, nullptr);
    EXPECT_TRUE(circle->GetIsTrigger());
    EXPECT_FLOAT_EQ(circle->GetRadius(), 0.42F);
    EXPECT_EQ(circle->GetCategoryBits(), 1u << 4);
}

TEST(Combat2DComponents, ProjectileDamagesHurtboxTarget) {
    Spark::GameWorld world{};
    constexpr std::uint16_t kHurtLayer = 1u << 4;

    Spark::GameObject* enemy = world.CreateGameObject();
    enemy->AddComponent<Spark::TransformComponent>()->SetTranslation({2.5F, 0.0F, 0.0F});
    enemy->AddComponent<Spark::Rigidbody2DComponent>(Spark::RigidbodyBodyType2D::Static, 0.0F);
    auto* hurtbox = enemy->AddComponent<Spark::Hurtbox2DComponent>();
    hurtbox->SetRadius(0.6F);
    hurtbox->SetCategoryBits(kHurtLayer);
    auto* health = enemy->AddComponent<Spark::HealthComponent>(5.0F);

    Spark::GameObject* shooter = world.CreateGameObject();
    Spark::GameObject* bullet = world.CreateGameObject();
    bullet->AddComponent<Spark::TransformComponent>()->SetTranslation({0.0F, 0.0F, 0.0F});
    auto* projectile = bullet->AddComponent<Spark::Projectile2DComponent>();
    projectile->SetRadius(0.2F);
    projectile->SetDamage(2.0F);
    projectile->SetBlockOnSolidHit(false);
    projectile->SetDestroyOwnerOnDeactivate(false);
    Spark::PhysicsQueryFilter2D filter{};
    filter.queryCategoryBits = 1u << 3;
    filter.queryMaskBits = kHurtLayer;
    filter.hitSolids = false;
    filter.hitTriggers = true;
    projectile->SetTargetFilter(filter);
    projectile->Activate({0.0F, 0.0F}, {30.0F, 0.0F}, shooter);

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.1F;
    projectile->OnUpdate(timing, *bullet, gTestEngineContext);

    EXPECT_LT(health->GetCurrent(), 5.0F);
    EXPECT_FALSE(projectile->IsActive());
}
