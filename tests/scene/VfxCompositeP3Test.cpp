#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/VfxPlayerComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"
#include "spark/scene/vfx/VfxAssetLoader.hpp"
#include "spark/scene/vfx/VfxEffectDefinition.hpp"

#include <cstdio>

namespace {

void RemoveIfExists(const char* path) {
    if (path != nullptr) {
        std::remove(path);
    }
}

}  // namespace

TEST(VfxCompositeTest, FireworksBuiltinIsComposite) {
    Spark::VfxAsset asset = Spark::VfxAsset::FromBuiltin("fireworks");
    EXPECT_TRUE(asset.IsComposite());
    EXPECT_EQ(asset.definition.emitters.GetSize(), 3u);
}

TEST(VfxCompositeTest, LoadsFireworksAssetFile) {
    Spark::GameWorldAssetCache cache{};
    const Spark::AssetLoadOutcome<Spark::VfxAsset> outcome =
            Spark::VfxAssetLoader::TryLoadFromFile("vfx/fireworks", cache);
    ASSERT_TRUE(outcome.ok);
    EXPECT_TRUE(outcome.value.IsComposite());
    EXPECT_EQ(outcome.value.definition.emitters.GetSize(), 3u);
    EXPECT_FLOAT_EQ(outcome.value.definition.emitters[1].startTimeSeconds, 0.28F);
    EXPECT_EQ(outcome.value.definition.emitters[1].burstCount, 96u);
}

TEST(VfxCompositeTest, PlayOnceSpawnsTrailImmediately) {
    Spark::GameWorld world{};
    Spark::GameObject* go = world.CreateGameObject();
    go->AddComponent<Spark::TransformComponent>();
    Spark::VfxPlayerComponent* player = go->AddComponent<Spark::VfxPlayerComponent>();
    player->SetVfxAssetKey("fireworks");
    player->PlayOnce(*go);

    bool trailActive = false;
    for (std::size_t i = 0; i < go->GetChildren().GetSize(); ++i) {
        Spark::GameObject* child = go->GetChildren()[i];
        if (const Spark::ParticleEmitterComponent* pe = child->GetComponent<Spark::ParticleEmitterComponent>()) {
            if (pe->IsEmitterEnabled() && pe->GetEmissionRate() > 0.0F) {
                trailActive = true;
            }
        }
    }
    EXPECT_TRUE(trailActive);
}

TEST(VfxCompositeTest, SavesAndReloadsCompositeAsset) {
    Spark::GameWorldAssetCache cache{};
    Spark::VfxAsset asset = Spark::VfxEffectDefinition::Fireworks().ToAsset();

    Spark::Utf8String diskPath(SPARK_BUILD_ASSETS_DIR);
    diskPath.AppendUtf8("/vfx/test_composite.sparkvfx");
    RemoveIfExists(diskPath.CStr());
    ASSERT_TRUE(Spark::VfxAssetLoader::TrySaveToFile(diskPath.CStr(), asset, SPARK_BUILD_ASSETS_DIR));

    const Spark::AssetLoadOutcome<Spark::VfxAsset> loaded =
            Spark::VfxAssetLoader::TryLoadFromFile("vfx/test_composite", cache);
    ASSERT_TRUE(loaded.ok);
    EXPECT_TRUE(loaded.value.IsComposite());
    EXPECT_EQ(loaded.value.definition.emitters.GetSize(), 3u);
    RemoveIfExists(diskPath.CStr());
}

TEST(VfxCompositeTest, ConfettiBuiltinResolves) {
    Spark::VfxAsset asset{};
    Spark::GameWorld world{};
    ASSERT_TRUE(Spark::VfxAsset::TryResolve("confetti", world, asset));
    EXPECT_TRUE(asset.IsComposite());
    EXPECT_GE(asset.definition.emitters.GetSize(), 1u);
}

TEST(VfxCompositeTest, MeteorStrikeBuiltinResolves) {
    Spark::VfxAsset asset = Spark::VfxAsset::FromBuiltin("meteor_strike");
    EXPECT_TRUE(asset.IsComposite());
    EXPECT_EQ(asset.definition.emitters.GetSize(), 3u);
    EXPECT_FLOAT_EQ(asset.definition.emitters[1].startTimeSeconds, 0.36F);
}

TEST(VfxCompositeTest, MagicImpactBuiltinResolves) {
    Spark::VfxAsset asset = Spark::VfxAsset::FromBuiltin("magic_impact");
    EXPECT_TRUE(asset.IsComposite());
    EXPECT_EQ(asset.definition.emitters.GetSize(), 3u);
}

TEST(VfxCompositeTest, SmokeGrenadeLoadsFromDisk) {
    Spark::GameWorldAssetCache cache{};
    const Spark::AssetLoadOutcome<Spark::VfxAsset> outcome =
            Spark::VfxAssetLoader::TryLoadFromFile("vfx/smoke_grenade", cache);
    ASSERT_TRUE(outcome.ok);
    EXPECT_TRUE(outcome.value.IsComposite());
    EXPECT_EQ(outcome.value.definition.emitters.GetSize(), 2u);
}
