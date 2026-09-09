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

TEST(SkinnedGltfPbrTest, PresenterAndApplicatorBindSkinnedMaterials) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.mesh));
    ASSERT_FALSE(asset.materials.IsEmpty());

    Spark::GameObject* owner = world.CreateGameObject();
    ASSERT_NE(owner, nullptr);
    owner->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);

    Spark::SkinnedGltfMaterialPresenter presenter{};
    presenter.PresentOn(*owner, asset, foxPath.CStr());

    const Spark::MaterialComponent* material = owner->GetComponent<Spark::MaterialComponent>();
    ASSERT_NE(material, nullptr);

    Spark::SceneRenderParams params{};
    const auto findTexture = [&params](const Spark::SharedPtr<Spark::Texture2D>& texture, Spark::Vector2* uvScale,
                                       Spark::Vector2* uvOffset) -> std::int32_t {
        return Spark::SceneSubmitDetail::FindOrAddSceneTexture(params, texture, uvScale, uvOffset);
    };
    Spark::SkinnedSceneDrawMaterialApplicator applicator(params, findTexture);

    Spark::SceneDrawItem item{};
    applicator.ApplyMaterial(item, *material);
    if (material->GetBaseColorTexture()) {
        EXPECT_GE(item.textureLayer, 0);
    }
    if (material->GetNormalTexture()) {
        EXPECT_GE(item.normalMapLayer, 0);
    }
    if (material->GetMetallicRoughnessTexture()) {
        EXPECT_GE(item.metallicRoughnessMapLayer, 0);
    }
}
