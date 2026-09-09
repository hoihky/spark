#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/ecs/components/animation/AnimatorComponent.hpp"
#include "spark/ecs/components/rendering/SkinnedMeshComponent.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/submit/SkinnedAnimationService.hpp"

#include <sys/stat.h>

namespace {

bool IsRegularFile(const char* path) {
    struct stat st {};
    return path != nullptr && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

}  // namespace

TEST(SkinnedAnimationBudgetTest, PaletteUpdateCapFallsBackToBindPose) {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(path.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));
    ASSERT_TRUE(static_cast<bool>(asset.mesh));

    Spark::GameObject* first = world.CreateGameObject();
    Spark::GameObject* second = world.CreateGameObject();
    first->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    second->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 2U, 1.0F);
    auto* meshA = first->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);
    auto* meshB = second->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);

    Spark::SkinnedAnimationService& service = world.GetSkinnedAnimationService();
    service.ResetPolicyToDefaults();
    service.GetBudget().SetMaxPaletteUpdatesPerFrame(1);
    service.GetBudget().SetMaxSkinnedDrawsPerFrame(4);
    service.GetPaletteCache().SetEnabled(false);
    service.BeginSubmitFrame(1);

    Spark::Array<Spark::Matrix4> paletteA;
    Spark::Array<Spark::Matrix4> paletteB;
    EXPECT_TRUE(service.TryResolvePalette(*meshA, first->GetComponent<Spark::AnimatorComponent>(), paletteA, 128));
    EXPECT_TRUE(service.TryResolvePalette(*meshB, second->GetComponent<Spark::AnimatorComponent>(), paletteB, 128));

    EXPECT_EQ(service.GetBudget().GetPaletteUpdatesUsed(), 1U);
    EXPECT_EQ(service.GetBudget().GetPaletteFallbacksToBindPose(), 1U);
    EXPECT_EQ(service.GetBudget().GetSkinnedDrawsSubmitted(), 2U);
}

TEST(SkinnedAnimationBudgetTest, SkinnedDrawCapSkipsExtraCharacters) {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(path.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));
    ASSERT_TRUE(static_cast<bool>(asset.mesh));

    Spark::GameObject* first = world.CreateGameObject();
    Spark::GameObject* second = world.CreateGameObject();
    first->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    second->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    auto* meshA = first->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);
    auto* meshB = second->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);

    Spark::SkinnedAnimationService& service = world.GetSkinnedAnimationService();
    service.ResetPolicyToDefaults();
    service.GetBudget().SetMaxPaletteUpdatesPerFrame(8);
    service.GetBudget().SetMaxSkinnedDrawsPerFrame(1);
    service.GetPaletteCache().SetEnabled(false);
    service.BeginSubmitFrame(2);

    Spark::Array<Spark::Matrix4> paletteA;
    Spark::Array<Spark::Matrix4> paletteB;
    EXPECT_TRUE(service.TryResolvePalette(*meshA, first->GetComponent<Spark::AnimatorComponent>(), paletteA, 128));
    EXPECT_FALSE(service.TryResolvePalette(*meshB, second->GetComponent<Spark::AnimatorComponent>(), paletteB, 128));
    EXPECT_EQ(service.GetBudget().GetSkinnedDrawsSubmitted(), 1U);
}

TEST(SkinnedAnimationBudgetTest, SharedSkeletonUsesPaletteCacheHit) {
    Spark::Utf8String path(SPARK_ASSETS_DIR);
    path.AppendUtf8("/models/Fox.glb");
    if (!IsRegularFile(path.CStr())) {
        GTEST_SKIP() << "Fox.glb not available";
    }

    Spark::GameWorld world{};
    const Spark::SkinnedGltfAsset asset = world.LoadSkinnedGltf(path.CStr());
    ASSERT_TRUE(static_cast<bool>(asset.skeleton));
    ASSERT_TRUE(static_cast<bool>(asset.mesh));

    Spark::GameObject* first = world.CreateGameObject();
    Spark::GameObject* second = world.CreateGameObject();
    auto* animA = first->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    auto* animB = second->AddComponent<Spark::AnimatorComponent>(asset.skeleton, 1U, 1.0F);
    animA->SetTimeSeconds(0.2F);
    animB->SetTimeSeconds(0.2F);
    auto* meshA = first->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);
    auto* meshB = second->AddComponent<Spark::SkinnedMeshComponent>(asset.mesh);

    Spark::SkinnedAnimationService& service = world.GetSkinnedAnimationService();
    service.ResetPolicyToDefaults();
    service.GetBudget().SetMaxPaletteUpdatesPerFrame(1);
    service.GetPaletteCache().SetEnabled(true);
    service.BeginSubmitFrame(3);

    Spark::Array<Spark::Matrix4> paletteA;
    Spark::Array<Spark::Matrix4> paletteB;
    EXPECT_TRUE(service.TryResolvePalette(*meshA, animA, paletteA, 128));
    EXPECT_TRUE(service.TryResolvePalette(*meshB, animB, paletteB, 128));

    EXPECT_EQ(service.GetBudget().GetPaletteUpdatesUsed(), 1U);
    EXPECT_EQ(service.GetPaletteCache().GetHitsThisFrame(), 1U);
    EXPECT_EQ(service.GetBudget().GetPaletteCacheHits(), 1U);
}
