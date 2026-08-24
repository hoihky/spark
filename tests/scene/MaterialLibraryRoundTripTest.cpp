#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/MultiMaterialComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/MaterialAsset.hpp"
#include "spark/scene/material/MaterialAssetLoader.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/MaterialSlotSnapshot.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/serialization/SceneSerializer.hpp"
#include "spark/scene/texture/Texture2D.hpp"

#include <cstdio>
#include <cstring>
#include <sys/stat.h>

namespace {

constexpr const char* kTestMaterialKey = "materials/test_roundtrip.sparkmat";

Spark::Utf8String TestMaterialDiskPath() {
    Spark::Utf8String path(SPARK_BUILD_ASSETS_DIR);
    path.AppendUtf8("/materials/test_roundtrip.sparkmat");
    return path;
}

void PumpUntilMaterialReady(Spark::GameWorld& world, const char* key, const int maxFrames = 200) {
    for (int frame = 0; frame < maxFrames; ++frame) {
        world.GetAssetLoader().Pump(world);
        if (world.IsMaterialReady(key)) {
            return;
        }
    }
}

}  // namespace

TEST(MaterialLibraryRoundTripTest, ResolveReadablePathFindsBuildAssetsMaterial) {
    const Spark::Utf8String diskPath = TestMaterialDiskPath();
    std::remove(diskPath.CStr());

    Spark::MaterialAsset asset{};
    asset.tint = {0.3F, 0.6F, 0.9F};
    asset.metallic = 0.42F;
    asset.roughness = 0.58F;
    ASSERT_TRUE(Spark::MaterialAssetLoader::TrySaveToFile(diskPath.CStr(), asset, SPARK_BUILD_ASSETS_DIR));

    const Spark::Utf8String resolved = Spark::MaterialAssetLoader::ResolveReadablePath(kTestMaterialKey);
    EXPECT_STREQ(resolved.CStr(), diskPath.CStr());

    std::remove(diskPath.CStr());
}

TEST(MaterialLibraryRoundTripTest, SparkMatSlotV1ParsesSavedScalars) {
    const char* line =
            "\"\" \"\" \"\" \"\" 0.800000 0.200000 0.150000 0.330000 0.670000 1.000000 1.000000 1.000000 "
            "1.000000 0.500000 0.100000 2.500000 1.000000 1.000000 1.000000 0 1.000000 0.000000 0 ";
    const char* cursor = line;
    Spark::MaterialSlotSnapshot::Data slot{};
    ASSERT_TRUE(Spark::MaterialSlotSnapshot::TryParseSlotV1(cursor, slot));
    EXPECT_FLOAT_EQ(slot.tint.x, 0.8F);
    EXPECT_FLOAT_EQ(slot.metallic, 0.33F);
    EXPECT_FLOAT_EQ(slot.emissiveIntensity, 2.5F);
}

TEST(MaterialLibraryRoundTripTest, SparkMatPersistsToonScalars) {
    const Spark::Utf8String diskPath = TestMaterialDiskPath();
    std::remove(diskPath.CStr());

    Spark::MaterialAsset source{};
    source.shadingModel = Spark::SceneShadingModel::ToonCel;
    source.toonDiffuseBands = 5;
    source.toonRimIntensity = 0.8F;
    source.toonRimPower = 3.2F;
    source.tint = {0.4F, 0.5F, 0.6F};
    ASSERT_TRUE(Spark::MaterialAssetLoader::TrySaveToFile(diskPath.CStr(), source, SPARK_BUILD_ASSETS_DIR));

    Spark::MaterialAssetFilePayload decoded{};
    ASSERT_TRUE(Spark::MaterialAssetLoader::TryDecodeFromFile(kTestMaterialKey, decoded));
    EXPECT_EQ(decoded.asset.shadingModel, Spark::SceneShadingModel::ToonCel);
    EXPECT_EQ(decoded.asset.toonDiffuseBands, 5);
    EXPECT_FLOAT_EQ(decoded.asset.toonRimIntensity, 0.8F);
    EXPECT_FLOAT_EQ(decoded.asset.toonRimPower, 3.2F);

    std::remove(diskPath.CStr());
}

TEST(MaterialLibraryRoundTripTest, SparkMatSaveLoadRoundTrip) {
    const Spark::Utf8String diskPath = TestMaterialDiskPath();
    std::remove(diskPath.CStr());

    Spark::GameWorld world{};
    Spark::MaterialAsset source{};
    source.name = Spark::Utf8String(kTestMaterialKey);
    source.tint = {0.8F, 0.2F, 0.15F};
    source.metallic = 0.33F;
    source.roughness = 0.67F;
    source.emissiveColor = {1.0F, 0.5F, 0.1F};
    source.emissiveIntensity = 2.5F;

    ASSERT_TRUE(world.SaveMaterialAsset(diskPath.CStr(), source, SPARK_BUILD_ASSETS_DIR));

    Spark::MaterialAssetFilePayload decoded{};
    ASSERT_TRUE(Spark::MaterialAssetLoader::TryDecodeFromFile(kTestMaterialKey, decoded))
            << "decode failed for saved sparkmat";

    const Spark::AssetLoadOutcome<Spark::MaterialAsset> loaded = world.TryLoadMaterial(kTestMaterialKey);
    ASSERT_TRUE(loaded.ok) << loaded.errorMessage.CStr();
    EXPECT_FLOAT_EQ(loaded.value.tint.x, source.tint.x);
    EXPECT_FLOAT_EQ(loaded.value.metallic, source.metallic);
    EXPECT_FLOAT_EQ(loaded.value.roughness, source.roughness);
    EXPECT_FLOAT_EQ(loaded.value.emissiveIntensity, source.emissiveIntensity);

    std::remove(diskPath.CStr());
}

TEST(MaterialLibraryRoundTripTest, MaterialComponentAssetAsyncApply) {
    const Spark::Utf8String diskPath = TestMaterialDiskPath();
    std::remove(diskPath.CStr());

    Spark::GameWorld world{};
    Spark::MaterialAsset source{};
    source.tint = {0.15F, 0.85F, 0.4F};
    source.metallic = 0.75F;
    source.roughness = 0.25F;
    ASSERT_TRUE(world.SaveMaterialAsset(diskPath.CStr(), source, SPARK_BUILD_ASSETS_DIR));

    Spark::GameObject* object = world.CreateGameObject();
    Spark::MaterialComponent* material = object->AddComponent<Spark::MaterialComponent>();
    ASSERT_NE(material, nullptr);
    material->SetMaterialAsset(world, kTestMaterialKey);
    EXPECT_TRUE(material->HasMaterialAsset());
    EXPECT_FLOAT_EQ(material->GetMetallic(), 0.0F);

    PumpUntilMaterialReady(world, kTestMaterialKey);
    material->TryApplyMaterialAsset(world);

    EXPECT_TRUE(world.IsMaterialReady(kTestMaterialKey));
    EXPECT_FLOAT_EQ(material->GetTint().x, source.tint.x);
    EXPECT_FLOAT_EQ(material->GetMetallic(), source.metallic);
    EXPECT_FLOAT_EQ(material->GetRoughness(), source.roughness);

    material->ClearMaterialAsset(world);
    world.DestroyGameObject(object);
    std::remove(diskPath.CStr());
}

TEST(MaterialLibraryRoundTripTest, AsyncLoadResolvesCacheKeyTexture) {
    const Spark::Utf8String diskPath = TestMaterialDiskPath();
    std::remove(diskPath.CStr());

    Spark::GameWorld world{};
    constexpr const char* kTextureKey = "test/procedural_base";
    Spark::SharedPtr<Spark::Texture2D> texture =
            Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String(kTextureKey));
    *texture = Spark::Texture2D::CreateCheckerboard(
            8, 2, Spark::Vector3{1.0F, 0.0F, 0.0F}, Spark::Vector3{0.0F, 0.0F, 1.0F});
    world.RegisterTexture(texture, kTextureKey);

    Spark::MaterialAsset source{};
    source.baseColor = texture;
    source.tint = {0.9F, 0.8F, 0.7F};
    source.metallic = 0.5F;
    source.roughness = 0.4F;
    ASSERT_TRUE(world.SaveMaterialAsset(diskPath.CStr(), source, SPARK_BUILD_ASSETS_DIR, &world.GetAssetCache()));

    Spark::MaterialAssetFilePayload decoded{};
    ASSERT_TRUE(Spark::MaterialAssetLoader::TryDecodeFromFile(kTestMaterialKey, decoded));
    EXPECT_FALSE(decoded.asset.baseColor);
    EXPECT_STREQ(decoded.slot.baseColorPath.CStr(), kTextureKey);

    world.RequestMaterial(kTestMaterialKey);
    PumpUntilMaterialReady(world, kTestMaterialKey);

    const Spark::MaterialAsset* loaded = world.TryGetMaterialByKeyOrPath(kTestMaterialKey);
    ASSERT_NE(loaded, nullptr);
    ASSERT_TRUE(loaded->baseColor);
    EXPECT_EQ(loaded->baseColor.Get(), texture.Get());
    EXPECT_FLOAT_EQ(loaded->tint.x, source.tint.x);
    EXPECT_FLOAT_EQ(loaded->metallic, source.metallic);

    std::remove(diskPath.CStr());
}

TEST(MaterialLibraryRoundTripTest, AsyncLoadResolvesTextureDisplayNameAlias) {
    const Spark::Utf8String diskPath = TestMaterialDiskPath();
    std::remove(diskPath.CStr());

    Spark::GameWorld world{};
    constexpr const char* kTextureKey = "spark/demo/display_alias";
    constexpr const char* kDisplayName = "KenneyBricks";
    Spark::SharedPtr<Spark::Texture2D> texture =
            Spark::MakeShared<Spark::Texture2D>(Spark::Utf8String(kDisplayName));
    *texture = Spark::Texture2D::CreateCheckerboard(
            8, 2, Spark::Vector3{1.0F, 0.0F, 0.0F}, Spark::Vector3{0.0F, 0.0F, 1.0F});
    world.RegisterTexture(texture, kTextureKey);

    Spark::MaterialAsset source{};
    source.baseColor = texture;
    source.tint = {0.4F, 0.5F, 0.6F};
    ASSERT_TRUE(world.SaveMaterialAsset(diskPath.CStr(), source, SPARK_BUILD_ASSETS_DIR));

    world.ReleaseAsset(Spark::CachedAssetKind::Material, kTestMaterialKey);
    world.InvalidateAssetLoadState(kTestMaterialKey, Spark::AssetLoadJobKind::Material);
    world.RequestMaterial(kTestMaterialKey);
    PumpUntilMaterialReady(world, kTestMaterialKey);

    const Spark::MaterialAsset* loaded = world.TryGetMaterialByKeyOrPath(kTestMaterialKey);
    ASSERT_NE(loaded, nullptr);
    ASSERT_TRUE(loaded->baseColor);
    EXPECT_EQ(loaded->baseColor.Get(), texture.Get());

    std::remove(diskPath.CStr());
}

TEST(MaterialLibraryRoundTripTest, SceneMaterialV4RoundTrip) {
    const Spark::Utf8String diskPath = TestMaterialDiskPath();
    std::remove(diskPath.CStr());

    Spark::GameWorld sourceWorld{};
    Spark::MaterialAsset sourceAsset{};
    sourceAsset.tint = {0.55F, 0.35F, 0.95F};
    sourceAsset.metallic = 0.18F;
    sourceAsset.roughness = 0.82F;
    ASSERT_TRUE(sourceWorld.SaveMaterialAsset(diskPath.CStr(), sourceAsset, SPARK_BUILD_ASSETS_DIR));
    sourceWorld.RegisterMaterial(sourceAsset, kTestMaterialKey);

    Spark::GameObject* sourceObject = sourceWorld.CreateGameObject();
    sourceObject->GetName() = Spark::Utf8String("MaterialRoundTripSource");
    Spark::MaterialComponent* sourceMaterial = sourceObject->AddComponent<Spark::MaterialComponent>();
    sourceMaterial->SetMaterialAsset(sourceWorld, kTestMaterialKey);
    sourceMaterial->SetTint({0.9F, 0.1F, 0.1F});

    Spark::SceneCaptureContext captureCtx{};
    Spark::ComponentRecord captured{};
    const Spark::IComponentSnapshotHandler* handler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::Material);
    ASSERT_NE(handler, nullptr);
    ASSERT_TRUE(handler->TryCapture(*sourceObject, captureCtx, captured));
    EXPECT_TRUE(std::strncmp(captured.payload.CStr(), "v4 \"", 4) == 0);

    Spark::GameWorld restoreWorld{};
    Spark::GameObject* restoreObject = restoreWorld.CreateGameObject();
    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = SPARK_BUILD_ASSETS_DIR;
    applyCtx.assetLoader = &restoreWorld.GetAssetLoader();
    ASSERT_TRUE(handler->TryRestore(*restoreObject, captured, restoreWorld, applyCtx));

    Spark::MaterialComponent* restored = restoreObject->GetComponent<Spark::MaterialComponent>();
    ASSERT_NE(restored, nullptr);
    EXPECT_STREQ(restored->GetMaterialAssetKey().CStr(), kTestMaterialKey);
    EXPECT_FLOAT_EQ(restored->GetTint().x, 0.9F);

    PumpUntilMaterialReady(restoreWorld, kTestMaterialKey);
    restored->TryApplyMaterialAsset(restoreWorld);
    EXPECT_FLOAT_EQ(restored->GetMetallic(), sourceAsset.metallic);

    restored->ClearMaterialAsset(restoreWorld);
    sourceMaterial->ClearMaterialAsset(sourceWorld);
    restoreWorld.DestroyGameObject(restoreObject);
    sourceWorld.DestroyGameObject(sourceObject);
    std::remove(diskPath.CStr());
}

TEST(MaterialLibraryRoundTripTest, MultiMaterialV2RoundTrip) {
    const Spark::Utf8String diskPath = TestMaterialDiskPath();
    std::remove(diskPath.CStr());

    Spark::GameWorld sourceWorld{};
    Spark::MaterialAsset sourceAsset{};
    sourceAsset.tint = {0.2F, 0.7F, 0.3F};
    sourceAsset.metallic = 0.55F;
    sourceAsset.roughness = 0.35F;
    sourceAsset.shadingModel = Spark::SceneShadingModel::ToonCel;
    sourceAsset.toonDiffuseBands = 4;
    sourceAsset.toonRimIntensity = 0.6F;
    sourceAsset.toonRimPower = 2.5F;
    ASSERT_TRUE(sourceWorld.SaveMaterialAsset(diskPath.CStr(), sourceAsset, SPARK_BUILD_ASSETS_DIR));
    sourceWorld.RegisterMaterial(sourceAsset, kTestMaterialKey);

    Spark::GameObject* sourceObject = sourceWorld.CreateGameObject();
    Spark::MultiMaterialComponent* sourceMulti = sourceObject->AddComponent<Spark::MultiMaterialComponent>();
    sourceMulti->ResizeSlots(2);
    sourceMulti->SetSlotMaterialAsset(sourceWorld, 0, kTestMaterialKey);
    sourceMulti->GetSlot(0).tint = {0.95F, 0.05F, 0.05F};
    sourceMulti->GetSlot(1).tint = {0.1F, 0.2F, 0.3F};
    sourceMulti->GetSlot(1).toonDiffuseBands = 2;
    sourceMulti->GetSlot(1).toonRimIntensity = 0.15F;
    sourceMulti->GetSlot(1).toonRimPower = 6.0F;

    Spark::SceneCaptureContext captureCtx{};
    Spark::ComponentRecord captured{};
    const Spark::IComponentSnapshotHandler* handler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::MultiMaterial);
    ASSERT_NE(handler, nullptr);
    ASSERT_TRUE(handler->TryCapture(*sourceObject, captureCtx, captured));
    EXPECT_TRUE(std::strncmp(captured.payload.CStr(), "v2 ", 3) == 0);

    Spark::GameWorld restoreWorld{};
    Spark::GameObject* restoreObject = restoreWorld.CreateGameObject();
    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = SPARK_BUILD_ASSETS_DIR;
    applyCtx.assetLoader = &restoreWorld.GetAssetLoader();
    ASSERT_TRUE(handler->TryRestore(*restoreObject, captured, restoreWorld, applyCtx));

    Spark::MultiMaterialComponent* restored = restoreObject->GetComponent<Spark::MultiMaterialComponent>();
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->GetSlotCount(), 2U);
    EXPECT_STREQ(restored->GetSlotMaterialAssetKey(0).CStr(), kTestMaterialKey);
    EXPECT_FALSE(restored->SlotHasMaterialAsset(1));
    EXPECT_EQ(restored->GetSlot(1).toonDiffuseBands, 2);
    EXPECT_FLOAT_EQ(restored->GetSlot(1).toonRimPower, 6.0F);
    EXPECT_FLOAT_EQ(restored->GetSlot(0).tint.x, 0.95F);
    EXPECT_FLOAT_EQ(restored->GetSlot(0).toonRimIntensity, 0.6F);

    PumpUntilMaterialReady(restoreWorld, kTestMaterialKey);
    restored->TryApplyMaterialAssets(restoreWorld);
    EXPECT_FLOAT_EQ(restored->GetSlot(0).metallic, sourceAsset.metallic);

    restored->ClearAllMaterialAssets(restoreWorld);
    sourceMulti->ClearAllMaterialAssets(sourceWorld);
    restoreWorld.DestroyGameObject(restoreObject);
    sourceWorld.DestroyGameObject(sourceObject);
    std::remove(diskPath.CStr());
}

TEST(MaterialLibraryRoundTripTest, GltfMaterialLibraryKeyFormat) {
    const Spark::Utf8String key =
            Spark::MaterialAssetLoader::MakeGltfMaterialLibraryKey("models/hero.glb", 2);
    EXPECT_STREQ(key.CStr(), "models/hero.glb#material/2");
}

TEST(MaterialLibraryRoundTripTest, LoadGltfRegistersMaterialLibraryKeys) {
    Spark::Utf8String gltfPath(SPARK_ASSETS_DIR);
    gltfPath.AppendUtf8("/models/DamagedHelmet.glb");
    struct stat st {};
    if (stat(gltfPath.CStr(), &st) != 0 || !S_ISREG(st.st_mode)) {
        GTEST_SKIP() << "DamagedHelmet.glb not available";
    }

    Spark::GameWorld world{};
    (void)world.LoadGltf(gltfPath.CStr());
    const Spark::Utf8String materialKey =
            Spark::MaterialAssetLoader::MakeGltfMaterialLibraryKey(gltfPath.CStr(), 0);
    const Spark::MaterialAsset* material = world.TryGetMaterialByKeyOrPath(materialKey.CStr());
    ASSERT_NE(material, nullptr);
    EXPECT_TRUE(material->HasAnyTexture());
}
