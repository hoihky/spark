#include <gtest/gtest.h>

#include "spark/animation/LocomotionClipSet.hpp"
#include "spark/config.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>
#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

void ExpectClipNamed(const Spark::Skeleton& skeleton, const std::uint32_t clipIndex, const char* expectedName) {
    ASSERT_LT(clipIndex, skeleton.GetClipCount());
    EXPECT_STRCASEEQ(skeleton.GetClipName(clipIndex).CStr(), expectedName);
}

TEST(SampleAssetCatalogTest, FoxMatchesDocumentedClips) {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(path.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));
    EXPECT_EQ(asset.skeleton->GetJointCount(), 24U);
    EXPECT_EQ(asset.skeleton->GetClipCount(), 3U);
    EXPECT_EQ(asset.walkClipIndex, 1U);

    ExpectClipNamed(*asset.skeleton, 0, "Survey");
    ExpectClipNamed(*asset.skeleton, 1, "Walk");
    ExpectClipNamed(*asset.skeleton, 2, "Run");
    EXPECT_GT(asset.skeleton->GetClipDuration(1), 0.5F);

    const Spark::LocomotionClipSet clips =
            Spark::ResolveLocomotionClipsFromSkeleton(*asset.skeleton, asset.walkClipIndex);
    EXPECT_EQ(clips.idle, 0U);
    EXPECT_EQ(clips.walk, 1U);
    EXPECT_EQ(clips.run, 2U);
}

TEST(SampleAssetCatalogTest, CesiumManMatchesDocumentedClips) {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/CesiumMan.glb");
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "CesiumMan.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(path.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));
    EXPECT_EQ(asset.skeleton->GetJointCount(), 19U);
    EXPECT_EQ(asset.skeleton->GetClipCount(), 1U);
    EXPECT_EQ(asset.walkClipIndex, 0U);
    EXPECT_GT(asset.skeleton->GetClipDuration(0), 0.1F);

    const Spark::LocomotionClipSet clips =
            Spark::ResolveLocomotionClipsFromSkeleton(*asset.skeleton, asset.walkClipIndex);
    EXPECT_EQ(clips.walk, 0U);
    EXPECT_EQ(clips.run, Spark::kInvalidAnimClipIndex);
}

}  // namespace
