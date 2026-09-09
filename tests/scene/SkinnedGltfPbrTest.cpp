#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/assets/gltf/SkinnedGltfMaterialPresenter.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/SkinnedSceneDrawMaterialApplicator.hpp"
#include "spark/scene/submit/detail/SceneSubmitDetail.hpp"

#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(SkinnedGltfPbrTest, SparkHumanoidLoadsNormalAndOrmTextures) {
    Spark::Utf8String humanoidPath(SPARK_ASSETS_DIR);
    humanoidPath.AppendUtf8("/models/SparkHumanoid.glb");
    if (!IsRegularFile(humanoidPath.CStr())) {
        GTEST_SKIP() << "SparkHumanoid.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(humanoidPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.mesh));
    ASSERT_FALSE(asset.materials.IsEmpty());

    const Spark::GltfMaterial& bodyMaterial = asset.materials[0];
    EXPECT_TRUE(static_cast<bool>(bodyMaterial.normalMap));
    EXPECT_TRUE(static_cast<bool>(bodyMaterial.metallicRoughness));
    EXPECT_GT(bodyMaterial.normalMap->GetWidth(), 0U);
    EXPECT_GT(bodyMaterial.metallicRoughness->GetWidth(), 0U);
}

TEST(SkinnedGltfPbrTest, PresenterAndApplicatorBindFullPbrLayers) {
    Spark::Utf8String humanoidPath(SPARK_ASSETS_DIR);
    humanoidPath.AppendUtf8("/models/SparkHumanoid.glb");
    if (!IsRegularFile(humanoidPath.CStr())) {
        GTEST_SKIP() << "SparkHumanoid.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(humanoidPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.mesh));
    ASSERT_FALSE(asset.materials.IsEmpty());
    ASSERT_TRUE(static_cast<bool>(asset.materials[0].normalMap));
    ASSERT_TRUE(static_cast<bool>(asset.materials[0].metallicRoughness));

    Spark::GameObject* owner = world.CreateGameObject();
    ASSERT_NE(owner, nullptr);
    owner->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);

    Spark::SkinnedGltfMaterialPresenter presenter{};
    presenter.PresentOn(*owner, asset, humanoidPath.CStr());

    const Spark::MaterialComponent* material = owner->GetComponent<Spark::MaterialComponent>();
    ASSERT_NE(material, nullptr);
    EXPECT_TRUE(static_cast<bool>(material->GetNormalTexture()));
    EXPECT_TRUE(static_cast<bool>(material->GetMetallicRoughnessTexture()));

    Spark::SceneRenderParams params{};
    const auto findTexture = [&params](const Spark::SharedPtr<Spark::Texture2D>& texture, Spark::Vector2* uvScale,
                                       Spark::Vector2* uvOffset) -> std::int32_t {
        return Spark::SceneSubmitDetail::FindOrAddSceneTexture(params, texture, uvScale, uvOffset);
    };
    Spark::SkinnedSceneDrawMaterialApplicator applicator(params, findTexture);

    Spark::SceneDrawItem item{};
    applicator.ApplyMaterial(item, *material);
    EXPECT_EQ(item.textureLayer, -1);
    EXPECT_GE(item.normalMapLayer, 0);
    EXPECT_GE(item.metallicRoughnessMapLayer, 0);
}
