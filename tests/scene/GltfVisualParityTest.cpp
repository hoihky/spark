#include <gtest/gtest.h>

#include "spark/animation/Skeleton.hpp"
#include "spark/config.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/render/scene/SceneShadingModel.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/assets/gltf/GltfPrimitiveDecoder.hpp"
#include "spark/scene/assets/gltf/GltfSceneLoader.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/GltfMaterial.hpp"

#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(GltfVisualParityTest, SkeletonJointBudgetRaisedTo128) {
    EXPECT_EQ(Spark::Skeleton::MaxJoints, 128U);
}

TEST(GltfVisualParityTest, UnlitShadingModelExists) {
    EXPECT_EQ(static_cast<int>(Spark::SceneShadingModel::Unlit), 2);
}

TEST(GltfVisualParityTest, FoxBindFromPathUsesSceneSkinnedImport) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    Spark::GameObject* owner = world.CreateGameObject();
    ASSERT_NE(owner, nullptr);
    ASSERT_TRUE(Spark::GltfAssetBinder::BindFromPath(*owner, foxPath.CStr()));

    int skinnedCount = 0;
    world.ForEachGameObject([&](Spark::GameObject* object) {
        if (object != nullptr && object->GetComponent<Spark::SkinnedMeshComponent>() != nullptr) {
            ++skinnedCount;
        }
    });
    EXPECT_GE(skinnedCount, 1);
}

TEST(GltfVisualParityTest, SceneDocumentLoadsSkinnedMeshesForFox) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    const Spark::AssetLoadOutcome<Spark::GltfSceneDocument> outcome =
            Spark::GltfSceneLoader::TryLoadFromFile(foxPath.CStr());
    ASSERT_TRUE(outcome.ok) << outcome.errorMessage.CStr();
    EXPECT_FALSE(outcome.value.skinnedMeshes.IsEmpty());
    EXPECT_FALSE(outcome.value.skeletons.IsEmpty());
    EXPECT_GT(outcome.value.skinnedMeshes[0]->GetVertices().GetSize(), 0U);
}

TEST(GltfVisualParityTest, DecodedPrimitiveSupportsColorAndSecondUv) {
    Spark::GltfDecodedPrimitive primitive{};
    primitive.positions.PushBack({0.0F, 0.0F, 0.0F});
    primitive.texcoords.PushBack({0.1F, 0.2F});
    primitive.texcoords1.PushBack({0.3F, 0.4F});
    primitive.colors.PushBack({0.5F, 0.6F, 0.7F, 1.0F});
    EXPECT_EQ(primitive.texcoords1.GetSize(), 1U);
    EXPECT_EQ(primitive.colors.GetSize(), 1U);
}

TEST(GltfVisualParityTest, GltfMaterialStoresPerMapUvMetadata) {
    Spark::GltfMaterial material{};
    material.baseColorUv.texCoordSet = 1;
    material.baseColorUv.uvScale = {2.0F, 3.0F};
    material.baseColorUv.uvOffset = {0.1F, 0.2F};
    material.baseColorUv.uvRotation = 0.5F;
    material.unlit = true;
    EXPECT_EQ(material.baseColorUv.texCoordSet, 1U);
    EXPECT_TRUE(material.unlit);
}
