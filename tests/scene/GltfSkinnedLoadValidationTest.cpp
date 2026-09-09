#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/assets/gltf/GltfSkinnedLoadValidator.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>
#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

[[nodiscard]] bool MatrixHasFiniteEntries(const Spark::Matrix4& matrix) {
    for (int i = 0; i < 16; ++i) {
        if (!std::isfinite(matrix.m[i])) {
            return false;
        }
    }
    return true;
}

}  // namespace

TEST(GltfSkinnedLoadValidationTest, FoxLoadsWithJointPaletteSample) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));
    ASSERT_TRUE(static_cast<bool>(asset.mesh));
    const std::uint32_t jointCount = asset.skeleton->GetJointCount();
    EXPECT_GT(jointCount, 0U);
    EXPECT_GT(asset.skeleton->GetClipCount(), 0U);
    EXPECT_TRUE(asset.skeleton->WasGltfInverseBindProvided());
    EXPECT_TRUE(asset.skeleton->HasValidInverseBindData());

    Spark::Array<Spark::Matrix4> palette;
    palette.Resize(jointCount);
    asset.skeleton->ComputePalette(0, 0.0F, palette.GetData(), jointCount);
    EXPECT_TRUE(MatrixHasFiniteEntries(palette[0]));
}

TEST(GltfSkinnedLoadValidationTest, ValidatorAcceptsFoxAsset) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));
    ASSERT_TRUE(static_cast<bool>(asset.mesh));

    Spark::GltfSkinnedLoadValidator validator(false);
    validator.ValidateSkinnedLoad(*asset.skeleton, *asset.mesh, foxPath.CStr());
    EXPECT_TRUE(validator.GetWarnings().IsEmpty());
}
