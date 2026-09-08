#include "spark/animation/Skeleton.hpp"
#include "spark/config.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/math/Matrix4.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

float MaxPaletteDiff(const Spark::Matrix4* a, const Spark::Matrix4* b, const std::uint32_t count) {
    float maxDiff = 0.0F;
    for (std::uint32_t j = 0; j < count; ++j) {
        for (int k = 0; k < 16; ++k) {
            maxDiff = std::max(maxDiff, std::fabs(a[j].m[k] - b[j].m[k]));
        }
    }
    return maxDiff;
}

}  // namespace

TEST(SkeletonPaletteBlendTest, BlendEndpointsMatchSingleClips) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    const std::int32_t walkIdx = asset.skeleton->FindClipIndexIfNameContains("walk");
    const std::int32_t runIdx = asset.skeleton->FindClipIndexIfNameContains("run");
    ASSERT_GE(walkIdx, 0);
    ASSERT_GE(runIdx, 0);

    const std::uint32_t jointCount = asset.skeleton->GetJointCount();
    const float sampleTime = 0.25F;
    Spark::Matrix4 walkPalette[Spark::Skeleton::MaxJoints]{};
    Spark::Matrix4 runPalette[Spark::Skeleton::MaxJoints]{};
    Spark::Matrix4 blendedPalette[Spark::Skeleton::MaxJoints]{};

    asset.skeleton->ComputePalette(static_cast<std::uint32_t>(walkIdx), sampleTime, walkPalette, jointCount);
    asset.skeleton->ComputePalette(static_cast<std::uint32_t>(runIdx), sampleTime, runPalette, jointCount);

    asset.skeleton->ComputeBlendedPalette(
            static_cast<std::uint32_t>(walkIdx),
            sampleTime,
            static_cast<std::uint32_t>(runIdx),
            sampleTime,
            0.0F,
            blendedPalette,
            jointCount);
    EXPECT_LT(MaxPaletteDiff(walkPalette, blendedPalette, jointCount), 1.0e-4F);

    asset.skeleton->ComputeBlendedPalette(
            static_cast<std::uint32_t>(walkIdx),
            sampleTime,
            static_cast<std::uint32_t>(runIdx),
            sampleTime,
            1.0F,
            blendedPalette,
            jointCount);
    EXPECT_LT(MaxPaletteDiff(runPalette, blendedPalette, jointCount), 1.0e-4F);
}

TEST(SkeletonPaletteBlendTest, AnimatorCrossfadeFromLocomotionBlend) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    const std::int32_t walkIdx = asset.skeleton->FindClipIndexIfNameContains("walk");
    const std::int32_t runIdx = asset.skeleton->FindClipIndexIfNameContains("run");
    ASSERT_GE(walkIdx, 0);
    ASSERT_GE(runIdx, 0);

    Spark::AnimatorComponent animator(
            asset.skeleton, static_cast<std::uint32_t>(walkIdx), 1.0F);
    animator.SetLocomotionBlend(
            static_cast<std::uint32_t>(walkIdx),
            static_cast<std::uint32_t>(runIdx),
            0.5F);
    animator.SetTimeSeconds(0.2F);

    const std::uint32_t jointCount = asset.skeleton->GetJointCount();
    Spark::Matrix4 blendedPalette[Spark::Skeleton::MaxJoints]{};
    animator.ComputeJointPalette(blendedPalette, jointCount);

    Spark::Matrix4 expectedPalette[Spark::Skeleton::MaxJoints]{};
    asset.skeleton->ComputeBlendedPalette(
            static_cast<std::uint32_t>(walkIdx),
            0.2F,
            static_cast<std::uint32_t>(runIdx),
            0.2F,
            0.5F,
            expectedPalette,
            jointCount);
    EXPECT_LT(MaxPaletteDiff(blendedPalette, expectedPalette, jointCount), 1.0e-3F);

    animator.SetClipIndexWithCrossfade(static_cast<std::uint32_t>(runIdx), 0.2F);
    Spark::Matrix4 crossfadePalette[Spark::Skeleton::MaxJoints]{};
    animator.ComputeJointPalette(crossfadePalette, jointCount);
    EXPECT_LT(MaxPaletteDiff(crossfadePalette, expectedPalette, jointCount), 1.0e-2F);
}
