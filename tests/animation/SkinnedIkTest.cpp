#include <gtest/gtest.h>

#include "spark/animation/ik/IkGroundProbe.hpp"
#include "spark/animation/ik/TwoBoneIkSolver.hpp"
#include "spark/config.hpp"
#include "spark/ecs/components/animation/AimIkComponent.hpp"
#include "spark/ecs/components/animation/FootIkComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/SkinnedIkService.hpp"

#include <cmath>
#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(SkinnedIkTest, GroundPlaneProbeHitsFallback) {
    Spark::IkGroundProbe probe{};
    probe.SetFallbackPlaneHeight(0.0F);
    Spark::IkGroundProbe::Hit hit{};
    EXPECT_TRUE(probe.ProbeDown({0.0F, 1.0F, 0.0F}, 2.0F, hit));
    EXPECT_TRUE(hit.hasHit);
    EXPECT_NEAR(hit.point.y, 0.0F, 1.0e-3F);
    EXPECT_NEAR(hit.normal.y, 1.0F, 1.0e-3F);
}

TEST(SkinnedIkTest, FoxFootIkResolvesDistinctHindLimbs) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    Spark::FootIkComponent footIk{};
    footIk.SetLeftFootPatterns("leftleg01", "leftleg02", "leftfoot");
    footIk.SetRightFootPatterns("rightleg01", "rightleg02", "rightfoot");
    EXPECT_TRUE(footIk.ConfigureFromSkeleton(*asset.skeleton));
    ASSERT_EQ(footIk.GetLimbs().GetSize(), 2U);
    EXPECT_NE(footIk.GetLimbs()[0].endJoint, footIk.GetLimbs()[1].endJoint);
    EXPECT_TRUE(footIk.GetLimbs()[0].hasMidJoint);
    EXPECT_TRUE(footIk.GetLimbs()[1].hasMidJoint);
    EXPECT_LT(footIk.GetLimbs()[0].poleBiasX, 0.0F);
    EXPECT_GT(footIk.GetLimbs()[1].poleBiasX, 0.0F);
}

TEST(SkinnedIkTest, AimIkConfiguresSpineChain) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    Spark::AimIkComponent aimIk{};
    const char* patterns[] = {"neck", "head"};
    aimIk.SetSpineJointPatterns(patterns, 2);
    EXPECT_TRUE(aimIk.ConfigureFromSkeleton(*asset.skeleton));
    EXPECT_GE(aimIk.GetSpineJointIndices().GetSize(), 1U);
}
