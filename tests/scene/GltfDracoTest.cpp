#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/components/rendering/MeshComponent.hpp"
#include "spark/scene/assets/gltf/GltfAssetBindings.hpp"
#include "spark/scene/assets/gltf/GltfContentClassifier.hpp"
#include "spark/scene/assets/gltf/GltfSceneDocument.hpp"
#include "spark/scene/assets/gltf/GltfSceneLoader.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

Spark::Utf8String AvocadoDracoPath() {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/AvocadoDraco/Avocado.gltf");
    return path;
}

}  // namespace

TEST(GltfDracoTest, ClassifierDetectsDracoCompression) {
    const Spark::Utf8String path = AvocadoDracoPath();
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "AvocadoDraco.gltf not available";
    }

    const Spark::GltfContentClassifier::ProbeResult probe = Spark::GltfContentClassifier::ProbeFile(path.CStr());
    ASSERT_TRUE(probe.parseOk);
    EXPECT_TRUE(probe.compression.draco);
    EXPECT_EQ(probe.kind, Spark::GltfContentKind::Rigid);
}

TEST(GltfDracoTest, SceneLoaderDecodesDracoMesh) {
    const Spark::Utf8String path = AvocadoDracoPath();
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "AvocadoDraco.gltf not available";
    }

#if !SPARK_ENABLE_GLTF_DRACO
    GTEST_SKIP() << "SPARK_ENABLE_GLTF_DRACO is disabled";
#endif

    const Spark::AssetLoadOutcome<Spark::GltfSceneDocument> outcome =
            Spark::GltfSceneLoader::TryLoadFromFile(path.CStr());
    ASSERT_TRUE(outcome.ok) << outcome.errorMessage.CStr();
    ASSERT_FALSE(outcome.value.meshes.IsEmpty());
    EXPECT_GT(outcome.value.meshes[0]->GetVertices().GetSize(), 0U);
    EXPECT_GT(outcome.value.meshes[0]->GetIndices().GetSize(), 0U);
}

TEST(GltfDracoTest, BindFromPathImportsDracoAsset) {
    const Spark::Utf8String path = AvocadoDracoPath();
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "AvocadoDraco.gltf not available";
    }

#if !SPARK_ENABLE_GLTF_DRACO
    GTEST_SKIP() << "SPARK_ENABLE_GLTF_DRACO is disabled";
#endif

    Spark::GameWorld world{};
    Spark::GameObject* owner = world.CreateGameObject();
    ASSERT_NE(owner, nullptr);
    ASSERT_TRUE(Spark::GltfAssetBinder::BindFromPath(*owner, path.CStr()));

    const Spark::MeshComponent* mesh = owner->GetComponent<Spark::MeshComponent>();
    ASSERT_NE(mesh, nullptr);
    EXPECT_GT(mesh->GetMesh()->GetVertices().GetSize(), 0U);
}
