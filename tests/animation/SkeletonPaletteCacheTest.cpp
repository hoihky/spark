#include <gtest/gtest.h>

#include "spark/animation/SkeletonPaletteCache.hpp"
#include "spark/animation/SkeletonPaletteCacheKey.hpp"
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

[[nodiscard]] bool PalettesEqual(
        const Spark::Array<Spark::Matrix4>& a,
        const Spark::Array<Spark::Matrix4>& b) {
    if (a.GetSize() != b.GetSize()) {
        return false;
    }
    for (std::size_t i = 0; i < a.GetSize(); ++i) {
        for (int k = 0; k < 16; ++k) {
            if (std::fabs(a[i].m[k] - b[i].m[k]) > 1.0e-4F) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

TEST(SkeletonPaletteCacheTest, IdenticalPlaybackSharesPalette) {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/SparkHumanoid.glb");
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "SparkHumanoid.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(path.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    Spark::GameObject* first = world.CreateGameObject();
    Spark::GameObject* second = world.CreateGameObject();
    auto* animA = first->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    auto* animB = second->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    animA->SetTimeSeconds(0.25F);
    animB->SetTimeSeconds(0.25F);

    Spark::SkeletonPaletteCache cache{};
    cache.SetQuantizationHz(30.0F);
    cache.BeginFrame(1);

    Spark::SkeletonPaletteCacheKey key{};
    key.CaptureFromAnimator(*animA, cache.GetQuantizationHz());

    Spark::Array<Spark::Matrix4> paletteA;
    paletteA.Resize(asset.skeleton->GetJointCount());
    animA->ComputeJointPalette(paletteA.GetData(), Spark::Skeleton::MaxJoints);
    cache.Store(key, paletteA);

    Spark::Array<Spark::Matrix4> paletteB;
    EXPECT_TRUE(cache.TryCopy(key, paletteB));
    EXPECT_TRUE(PalettesEqual(paletteA, paletteB));
}

TEST(SkeletonPaletteCacheTest, DifferentQuantizedTimeMissesCache) {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/SparkHumanoid.glb");
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "SparkHumanoid.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(path.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));

    Spark::GameObject* first = world.CreateGameObject();
    Spark::GameObject* second = world.CreateGameObject();
    auto* animA = first->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    auto* animB = second->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    animA->SetTimeSeconds(0.10F);
    animB->SetTimeSeconds(0.90F);

    Spark::SkeletonPaletteCache cache{};
    cache.SetQuantizationHz(2.0F);
    cache.BeginFrame(2);

    Spark::SkeletonPaletteCacheKey keyA{};
    keyA.CaptureFromAnimator(*animA, cache.GetQuantizationHz());
    Spark::SkeletonPaletteCacheKey keyB{};
    keyB.CaptureFromAnimator(*animB, cache.GetQuantizationHz());
    EXPECT_FALSE(keyA.Matches(keyB));

    Spark::Array<Spark::Matrix4> paletteA;
    paletteA.Resize(asset.skeleton->GetJointCount());
    animA->ComputeJointPalette(paletteA.GetData(), Spark::Skeleton::MaxJoints);
    cache.Store(keyA, paletteA);

    Spark::Array<Spark::Matrix4> paletteB;
    EXPECT_FALSE(cache.TryCopy(keyB, paletteB));
}
