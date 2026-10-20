#include <gtest/gtest.h>

#include <exception>

#include "spark/ecs/components/gameplay/CombatHurtFlash3DComponent.hpp"
#include "spark/ecs/components/gameplay/HealthComponent.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/physics/3d/Hurtbox3DComponent.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/gameplay/Combat3D.hpp"
#include "spark/physics/CollisionFilter3D.hpp"
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

TEST(Combat3DComponents, HurtboxInvulnerabilityBlocksDamage) {
    Spark::GameWorld world{};
    Spark::GameObject* victim = world.CreateGameObject();
    victim->AddComponent<Spark::HealthComponent>(10.0F);
    auto* hurtbox = victim->AddComponent<Spark::Hurtbox3DComponent>();
    hurtbox->SetInvulnerabilitySeconds(0.5F);
    hurtbox->NotifyDamageReceived();

    EXPECT_TRUE(hurtbox->IsInvulnerable());
    const float blocked = Spark::Combat3DTarget{*victim}.ApplyDamage(nullptr, 3.0F);
    EXPECT_FLOAT_EQ(blocked, 0.0F);

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.6F;
    hurtbox->OnUpdate(timing, *victim, gTestEngineContext);

    EXPECT_FALSE(hurtbox->IsInvulnerable());
    const float applied = Spark::Combat3DTarget{*victim}.ApplyDamage(nullptr, 3.0F);
    EXPECT_GT(applied, 0.0F);
}

TEST(Combat3DComponents, LayerFilteredHurtboxQueryFindsEnemy) {
    Spark::GameWorld world{};
    Spark::GameObject* enemy = world.CreateGameObject();
    auto* hurtbox = enemy->AddComponent<Spark::Hurtbox3DComponent>();
    hurtbox->SetCategoryBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Enemy));
    hurtbox->SetMaskBits(Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Player));

    Spark::PhysicsQueryFilter3D filter{};
    filter.queryCategoryBits = Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Player);
    filter.queryMaskBits = Spark::CollisionFilter3D::LayerBit(Spark::PhysicsLayers3D::Enemy);

    Spark::CombatHurtboxSphereQuery query{world};
    query.SetFilter(filter);
    Spark::Array<Spark::CombatHurtboxOverlap> hits{};
    query.CollectOverlaps(hurtbox->GetWorldCenter(*enemy), 1.0F, hits);
    EXPECT_EQ(hits.GetSize(), 1u);
    EXPECT_EQ(hits[0].GetOwner(), enemy);
}

TEST(Combat3DComponents, HurtFlashTintsOnDamageAndRestores) {
    Spark::GameWorld world{};
    Spark::GameObject* dummy = world.CreateGameObject();
    auto* health = dummy->AddComponent<Spark::HealthComponent>(20.0F);
    auto* material = dummy->AddComponent<Spark::MaterialComponent>();
    material->SetTint(Spark::Vector3{0.5F, 0.5F, 0.5F});
    auto* flash = dummy->AddComponent<Spark::CombatHurtFlash3DComponent>();
    flash->SetFlashDuration(0.1F);
    flash->SetHurtTint(Spark::Vector3{1.0F, 0.0F, 0.0F});

    health->ApplyDamage(5.0F, nullptr);
    const Spark::Vector3 flashTint = material->GetTint();
    EXPECT_FLOAT_EQ(flashTint.x, 1.0F);
    EXPECT_FLOAT_EQ(flashTint.y, 0.0F);
    EXPECT_FLOAT_EQ(flashTint.z, 0.0F);

    Spark::FrameTiming timing{};
    timing.deltaTimeSeconds = 0.15F;
    flash->OnUpdate(timing, *dummy, gTestEngineContext);
    const Spark::Vector3 restored = material->GetTint();
    EXPECT_FLOAT_EQ(restored.x, 0.5F);
    EXPECT_FLOAT_EQ(restored.y, 0.5F);
    EXPECT_FLOAT_EQ(restored.z, 0.5F);
}
