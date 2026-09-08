#include <gtest/gtest.h>

#include "spark/animation/IRootMotionJointResolver.hpp"
#include "spark/animation/RootMotionSampler.hpp"
#include "spark/config.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"

#include <cmath>
#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(RootMotionJointResolverTest, PatternFindsFoxHipJoint) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    Spark::PatternRootMotionJointResolver resolver{};
    const std::uint32_t joint = resolver.ResolveJointIndex(*asset.skeleton);
    EXPECT_LT(joint, asset.skeleton->GetJointCount());
    EXPECT_GE(asset.skeleton->FindJointIndexIfNameContains("hip"), 0);
}

TEST(RootMotionSamplerTest, WalkClipProducesNonZeroHorizontalDelta) {
    Spark::Utf8String foxPath(SPARK_ASSETS_DIR);
    foxPath.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(foxPath.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(foxPath.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    const std::int32_t walkIdx = asset.skeleton->FindClipIndexIfNameContains("walk");
    ASSERT_GE(walkIdx, 0);

    Spark::PatternRootMotionJointResolver resolver{};
    const std::uint32_t joint = resolver.ResolveJointIndex(*asset.skeleton);

    Spark::AnimatorComponent animator(asset.skeleton, static_cast<std::uint32_t>(walkIdx), 1.0F);
    animator.SetLoopMode(Spark::AnimLoopMode::Loop);

    Spark::RootMotionSampler sampler{};
    Spark::Vector3 previous{};
    bool hasPrevious = false;
    Spark::RootMotionDelta delta{};

    EXPECT_TRUE(sampler.TryComputeDelta(animator, joint, 1.0F / 30.0F, previous, hasPrevious, delta));
    EXPECT_FALSE(delta.valid);

    float maxHorizontal = 0.0F;
    for (int frame = 1; frame <= 30; ++frame) {
        animator.SetTimeSeconds(static_cast<float>(frame) / 30.0F);
        if (sampler.TryComputeDelta(animator, joint, 1.0F / 30.0F, previous, hasPrevious, delta) && delta.valid) {
            const float horizontal = std::sqrt(delta.translation.x * delta.translation.x
                    + delta.translation.z * delta.translation.z);
            maxHorizontal = std::max(maxHorizontal, horizontal);
        }
    }

    EXPECT_GT(maxHorizontal, 1.0e-4F);
}
