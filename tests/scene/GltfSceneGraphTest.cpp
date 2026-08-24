#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/components/core/TransformComponent.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/assets/gltf/GltfSceneImporter.hpp"
#include "spark/scene/assets/gltf/GltfSceneLoader.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <string_view>
#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(GltfSceneGraphTest, SceneLoaderPreservesNodesAndMeshes) {
    Spark::Utf8String treePath(SPARK_ASSETS_DIR);
    treePath.AppendUtf8("/models/Tree_1_C_Color1.gltf");
    if (!IsRegularFile(treePath.CStr())) {
        GTEST_SKIP() << "Tree_1_C_Color1.gltf not available";
    }

    const Spark::AssetLoadOutcome<Spark::GltfSceneDocument> outcome =
            Spark::GltfSceneLoader::TryLoadFromFile(treePath.CStr());
    ASSERT_TRUE(outcome.ok);
    EXPECT_EQ(outcome.value.nodes.GetSize(), 1U);
    EXPECT_EQ(outcome.value.rootNodeIndices.GetSize(), 1U);
    EXPECT_EQ(outcome.value.meshes.GetSize(), 1U);
    EXPECT_FALSE(outcome.value.materials.IsEmpty());
    EXPECT_TRUE(outcome.value.nodes[0].HasMesh());
}

TEST(GltfSceneGraphTest, ImporterPreservesParentChildHierarchy) {
    Spark::GltfSceneDocument document{};
    document.sourcePath = Spark::Utf8String("synthetic");

    Spark::GltfSceneNode root{};
    root.name = Spark::Utf8String("Root");
    root.localTransform = Spark::Transform::FromTranslation({2.0F, 0.0F, 0.0F});
    root.parentIndex = -1;

    Spark::GltfSceneNode child{};
    child.name = Spark::Utf8String("Child");
    child.parentIndex = 0;
    child.localTransform = Spark::Transform::FromTranslation({0.0F, 1.0F, 0.0F});

    document.nodes.PushBack(root);
    document.nodes.PushBack(child);
    document.rootNodeIndices.PushBack(0);

    Spark::GameWorld world{};
    Spark::GameObject* owner = world.CreateGameObject();
    ASSERT_NE(owner, nullptr);
    ASSERT_TRUE(Spark::GltfSceneImporter::ImportInto(*owner, document));

    EXPECT_EQ(owner->GetName().CStr(), std::string_view{"Root"});
    const Spark::TransformComponent* rootTransform = owner->GetComponent<Spark::TransformComponent>();
    ASSERT_NE(rootTransform, nullptr);
    EXPECT_FLOAT_EQ(rootTransform->GetLocalTransform().translation.x, 2.0F);

    ASSERT_EQ(owner->GetChildren().GetSize(), 1U);
    Spark::GameObject* childObject = owner->GetChildren()[0];
    ASSERT_NE(childObject, nullptr);
    EXPECT_EQ(childObject->GetName().CStr(), std::string_view{"Child"});
    const Spark::TransformComponent* childTransform = childObject->GetComponent<Spark::TransformComponent>();
    ASSERT_NE(childTransform, nullptr);
    EXPECT_FLOAT_EQ(childTransform->GetLocalTransform().translation.y, 1.0F);
}

TEST(GltfSceneGraphTest, BindFromPathImportsRigidSceneOntoOwner) {
    Spark::Utf8String treePath(SPARK_ASSETS_DIR);
    treePath.AppendUtf8("/models/Tree_1_C_Color1.gltf");
    if (!IsRegularFile(treePath.CStr())) {
        GTEST_SKIP() << "Tree_1_C_Color1.gltf not available";
    }

    Spark::GameWorld world{};
    Spark::GameObject* owner = world.CreateGameObject();
    ASSERT_NE(owner, nullptr);
    ASSERT_TRUE(Spark::GltfAssetBinder::BindFromPath(*owner, treePath.CStr()));

    const Spark::MeshComponent* mesh = owner->GetComponent<Spark::MeshComponent>();
    ASSERT_NE(mesh, nullptr);
    EXPECT_TRUE(static_cast<bool>(mesh->GetMesh()));
    EXPECT_GT(mesh->GetMesh()->GetVertices().GetSize(), 0U);
}

TEST(GltfSceneGraphTest, SceneCacheReturnsSameDocument) {
    Spark::Utf8String treePath(SPARK_ASSETS_DIR);
    treePath.AppendUtf8("/models/Tree_1_C_Color1.gltf");
    if (!IsRegularFile(treePath.CStr())) {
        GTEST_SKIP() << "Tree_1_C_Color1.gltf not available";
    }

    Spark::GameWorld world{};
    const Spark::AssetLoadOutcome<Spark::GltfSceneDocument> first = world.TryLoadGltfScene(treePath.CStr());
    const Spark::AssetLoadOutcome<Spark::GltfSceneDocument> second = world.TryLoadGltfScene(treePath.CStr());
    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(first.value.nodes.GetSize(), second.value.nodes.GetSize());
    EXPECT_EQ(first.value.meshes.GetSize(), second.value.meshes.GetSize());
    if (!first.value.meshes.IsEmpty() && !second.value.meshes.IsEmpty()) {
        EXPECT_EQ(first.value.meshes[0].Get(), second.value.meshes[0].Get());
    }
}
