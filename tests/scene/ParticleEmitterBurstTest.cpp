#include <gtest/gtest.h>

#include "spark/core/Array.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/vfx/VfxLibrary.hpp"

TEST(ParticleEmitterBurstTest, BurstSpawnsRequestedParticles) {
    Spark::GameWorld world{};
    Spark::GameObject* go = world.CreateGameObject();
    go->AddComponent<Spark::TransformComponent>();

    Spark::ParticleEmitterComponent* pe = go->AddComponent<Spark::ParticleEmitterComponent>();
    Spark::VfxLibrary::ApplyBuiltin(Spark::VfxBuiltinId::Explosion, *pe);
    pe->Burst(*go, 32);

    Spark::Array<Spark::SceneParticleInstance> instances{};
    pe->CollectInstances(instances);
    EXPECT_EQ(instances.GetSize(), 32u);
}

TEST(ParticleEmitterBurstTest, ExtendedBuiltinNamesResolve) {
    EXPECT_TRUE(Spark::VfxLibrary::IsBuiltinName("rain"));
    EXPECT_TRUE(Spark::VfxLibrary::IsBuiltinName("muzzle_flash"));
    EXPECT_TRUE(Spark::VfxLibrary::IsBuiltinName("heal"));
    EXPECT_TRUE(Spark::VfxLibrary::IsBuiltinName("loot_sparkle"));
    EXPECT_TRUE(Spark::VfxLibrary::IsBuiltinName("magic_bolt"));
    EXPECT_TRUE(Spark::VfxLibrary::IsBuiltinName("meteor_trail"));
    EXPECT_TRUE(Spark::VfxLibrary::IsBuiltinName("shockwave"));
    EXPECT_EQ(Spark::VfxLibrary::GetDefaultBurstCount(Spark::VfxBuiltinId::MuzzleFlash), 28u);
    EXPECT_EQ(Spark::VfxLibrary::GetDefaultBurstCount(Spark::VfxBuiltinId::Explosion), 112u);
}

TEST(ParticleEmitterBurstTest, BuiltinNamesResolve) {
    Spark::GameWorld world{};
    Spark::GameObject* go = world.CreateGameObject();
    Spark::ParticleEmitterComponent* pe = go->AddComponent<Spark::ParticleEmitterComponent>();

    EXPECT_TRUE(Spark::VfxLibrary::TryApplyBuiltinByName("explosion", *pe));
    EXPECT_FLOAT_EQ(pe->GetEmissionRate(), 0.0F);
    EXPECT_TRUE(pe->GetUseLocalEmission());
}
