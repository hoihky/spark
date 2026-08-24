#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/scene/assets/gltf/GltfPrimitiveDecoder.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(GltfSkinnedCompressionTest, FoxSkinnedMeshStillLoads) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.mesh));
    EXPECT_GT(asset.mesh->GetVertices().GetSize(), 0U);
    EXPECT_TRUE(static_cast<bool>(asset.skeleton));
    EXPECT_GT(asset.skeleton->GetJointCount(), 0U);
}

TEST(GltfSkinnedCompressionTest, DecoderRegistryReadsSkinnedAttributes) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.mesh));
    const Spark::SkinnedMesh::Vertex& vertex = asset.mesh->GetVertices()[0];
    const float weightSum = vertex.weights[0] + vertex.weights[1] + vertex.weights[2] + vertex.weights[3];
    EXPECT_GT(weightSum, 0.99F);
    EXPECT_LT(weightSum, 1.01F);
}
