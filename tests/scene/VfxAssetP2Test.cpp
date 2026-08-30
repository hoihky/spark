#include <cstdio>

#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/core/Array.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/ecs/components/rendering/VfxPlayerComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/assets/GameWorldAssetCache.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/vfx/VfxAsset.hpp"
#include "spark/scene/vfx/VfxAssetLoader.hpp"
#include "spark/scene/vfx/VfxSubsystemProcess.hpp"

namespace {

void RemoveIfExists(const char* path) {
    if (path != nullptr) {
        std::remove(path);
    }
}

Spark::Utf8String TestVfxDiskPath() {
    Spark::Utf8String path(SPARK_BUILD_ASSETS_DIR);
    path.AppendUtf8("/vfx/test_roundtrip.sparkvfx");
    return path;
}

}  // namespace

TEST(VfxAssetLoaderTest, LoadsBuiltinExplosionAsset) {
    Spark::GameWorldAssetCache cache{};
    const Spark::AssetLoadOutcome<Spark::VfxAsset> outcome =
            Spark::VfxAssetLoader::TryLoadFromFile("vfx/explosion", cache);
    ASSERT_TRUE(outcome.ok);
    EXPECT_STREQ(outcome.value.builtinName.CStr(), "explosion");
    EXPECT_EQ(outcome.value.burstCount, 112u);
}

TEST(VfxAssetLoaderTest, SavesAndReloadsCustomAsset) {
    Spark::GameWorldAssetCache cache{};
    Spark::VfxAsset asset{};
    asset.emitter.maxParticles = 128;
    asset.emitter.emissionRate = 12.0F;
    asset.emitter.lifeMin = 0.2F;
    asset.emitter.lifeMax = 0.5F;
    asset.burstCount = 8;

    const Spark::Utf8String diskPath = TestVfxDiskPath();
    RemoveIfExists(diskPath.CStr());
    ASSERT_TRUE(Spark::VfxAssetLoader::TrySaveToFile(diskPath.CStr(), asset, SPARK_BUILD_ASSETS_DIR));
    const Spark::AssetLoadOutcome<Spark::VfxAsset> loaded =
            Spark::VfxAssetLoader::TryLoadFromFile("vfx/test_roundtrip", cache);
    ASSERT_TRUE(loaded.ok);
    EXPECT_TRUE(loaded.value.builtinName.IsEmpty());
    EXPECT_EQ(loaded.value.burstCount, 8u);
    EXPECT_FLOAT_EQ(loaded.value.emitter.emissionRate, 12.0F);
    RemoveIfExists(diskPath.CStr());
}

TEST(VfxPlayerComponentTest, PlayOnceSpawnsBurstParticles) {
    Spark::GameWorld world{};
    Spark::GameObject* go = world.CreateGameObject();
    go->AddComponent<Spark::TransformComponent>();
    Spark::VfxPlayerComponent* player = go->AddComponent<Spark::VfxPlayerComponent>();
    player->SetVfxAssetKey("explosion");
    player->PlayOnce(*go);

    Spark::ParticleEmitterComponent* pe = go->GetComponent<Spark::ParticleEmitterComponent>();
    ASSERT_NE(pe, nullptr);
    EXPECT_GT(pe->GetAliveParticleCount(), 0u);
}

TEST(VfxPlayerComponentTest, SnapshotRoundTrip) {
    Spark::GameWorld world{};
    Spark::GameObject* source = world.CreateGameObject();
    Spark::GameObject* restored = world.CreateGameObject();
    Spark::VfxPlayerComponent* player = source->AddComponent<Spark::VfxPlayerComponent>();
    player->SetVfxAssetKey("vfx/impact");
    player->SetPlayOnStartOnce(true);

    const Spark::IComponentSnapshotHandler* handler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::VfxPlayer);
    ASSERT_NE(handler, nullptr);
    Spark::SceneCaptureContext captureCtx{};
    Spark::ComponentRecord captured{};
    ASSERT_TRUE(handler->TryCapture(*source, captureCtx, captured));

    Spark::SceneApplyContext applyCtx{};
    ASSERT_TRUE(handler->TryRestore(*restored, captured, world, applyCtx));

    const Spark::VfxPlayerComponent* restoredPlayer = restored->GetComponent<Spark::VfxPlayerComponent>();
    ASSERT_NE(restoredPlayer, nullptr);
    EXPECT_STREQ(restoredPlayer->GetVfxAssetKey().CStr(), "vfx/impact");
    EXPECT_TRUE(restoredPlayer->GetPlayOnStartOnce());
}

TEST(VfxSubsystemTest, QueuedOneShotCreatesParticles) {
    Spark::GameWorld world{};
    world.GetVfxSubsystem().Queue("explosion", {1.0F, 2.0F, 3.0F});
    Spark::ProcessVfx(world);

    Spark::Array<Spark::SceneParticleInstance> instances{};
    world.ForEachActiveGameObject([&instances](Spark::GameObject* object) {
        if (object == nullptr) {
            return;
        }
        if (object->GetName() != Spark::Utf8String("__vfx_pooled")) {
            return;
        }
        if (const Spark::ParticleEmitterComponent* pe = object->GetComponent<Spark::ParticleEmitterComponent>()) {
            pe->CollectInstances(instances);
        }
    });
    EXPECT_GT(instances.GetSize(), 0u);
}
