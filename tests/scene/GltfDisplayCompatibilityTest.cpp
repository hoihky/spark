#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/scene/assets/AssetLoadOutcome.hpp"
#include "spark/scene/assets/gltf/GltfContentClassifier.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/material/GltfMaterial.hpp"

#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(GltfDisplayCompatibilityTest, FactorOnlyMaterialHasPresentationContent) {
    Spark::GltfMaterial material{};
    material.baseColorFactor = {0.2F, 0.6F, 0.9F};
    material.metallicFactor = 0.35F;
    material.roughnessFactor = 0.75F;
    EXPECT_FALSE(material.HasAnyTexture());
    EXPECT_TRUE(material.HasScalarPresentation());
    EXPECT_TRUE(material.HasPresentationContent());
}

TEST(GltfDisplayCompatibilityTest, DefaultMaterialHasNoScalarPresentation) {
    Spark::GltfMaterial material{};
    EXPECT_FALSE(material.HasScalarPresentation());
    EXPECT_FALSE(material.HasPresentationContent());
}

TEST(GltfDisplayCompatibilityTest, ClassifiesRigidAndSkinnedAssets) {
    Spark::Utf8String helmetPath(SPARK_ASSETS_DIR);
    helmetPath.AppendUtf8("/models/DamagedHelmet.glb");
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");

    if (!IsRegularFile(helmetPath.CStr()) || !IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Sample glTF assets not available";
    }

    const Spark::GltfContentClassifier::ProbeResult helmet = Spark::GltfContentClassifier::ProbeFile(helmetPath.CStr());
    ASSERT_TRUE(helmet.parseOk);
    EXPECT_EQ(helmet.kind, Spark::GltfContentKind::Rigid);

    const Spark::GltfContentClassifier::ProbeResult fox = Spark::GltfContentClassifier::ProbeFile(foxPath.CStr());
    ASSERT_TRUE(fox.parseOk);
    EXPECT_EQ(fox.kind, Spark::GltfContentKind::Skinned);
}

TEST(GltfDisplayCompatibilityTest, RigidLoaderRejectsSkinnedFile) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::AssetLoadOutcome<Spark::GltfAsset> outcome = world.TryLoadGltf(foxPath.CStr());
    EXPECT_FALSE(outcome.ok);
    EXPECT_FALSE(outcome.errorMessage.IsEmpty());
}
