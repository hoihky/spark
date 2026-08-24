#include <gtest/gtest.h>

#include "spark/config.hpp"
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

Spark::Utf8String MeshoptCubePath() {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/MeshoptCubeTest/MeshoptCubeTest.gltf");
    return path;
}

}  // namespace

TEST(GltfMeshoptTest, ClassifierDetectsMeshoptCompression) {
    const Spark::Utf8String path = MeshoptCubePath();
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "MeshoptCubeTest.gltf not available";
    }

    const Spark::GltfContentClassifier::ProbeResult probe = Spark::GltfContentClassifier::ProbeFile(path.CStr());
    ASSERT_TRUE(probe.parseOk);
    EXPECT_TRUE(probe.compression.meshopt);
}

TEST(GltfMeshoptTest, SceneLoaderDecodesMeshoptAsset) {
    const Spark::Utf8String path = MeshoptCubePath();
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "MeshoptCubeTest.gltf not available";
    }

#if !SPARK_ENABLE_GLTF_MESHOPT
    GTEST_SKIP() << "SPARK_ENABLE_GLTF_MESHOPT is disabled";
#endif

    const Spark::AssetLoadOutcome<Spark::GltfSceneDocument> outcome =
            Spark::GltfSceneLoader::TryLoadFromFile(path.CStr());
    ASSERT_TRUE(outcome.ok) << outcome.errorMessage.CStr();
    ASSERT_FALSE(outcome.value.meshes.IsEmpty());
    EXPECT_GT(outcome.value.meshes[0]->GetVertices().GetSize(), 0U);
}

TEST(GltfMeshoptTest, BindFromPathImportsMeshoptAsset) {
    const Spark::Utf8String path = MeshoptCubePath();
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "MeshoptCubeTest.gltf not available";
    }

#if !SPARK_ENABLE_GLTF_MESHOPT
    GTEST_SKIP() << "SPARK_ENABLE_GLTF_MESHOPT is disabled";
#endif

    Spark::GameWorld world{};
    Spark::GameObject* owner = world.CreateGameObject();
    ASSERT_NE(owner, nullptr);
    ASSERT_TRUE(Spark::GltfAssetBinder::BindFromPath(*owner, path.CStr()));
    EXPECT_FALSE(owner->GetChildren().IsEmpty());
}
