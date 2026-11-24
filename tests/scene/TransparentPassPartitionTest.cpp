#include <gtest/gtest.h>

#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/VulkanSceneOpaqueSnapshot.hpp"
#include "spark/scene/submit/SceneDrawListSorter.hpp"
#include "spark/scene/submit/SceneRenderQueue.hpp"

namespace {

constexpr Spark::Vector3 kCam{0.0F, 0.0F, 0.0F};

[[nodiscard]] float SquaredDistanceFromCamera(const Spark::SceneDrawItem& item, const Spark::Vector3& camera) noexcept {
    const float dx = item.model.m[12] - camera.x;
    const float dy = item.model.m[13] - camera.y;
    const float dz = item.model.m[14] - camera.z;
    return dx * dx + dy * dy + dz * dz;
}

void SetTranslation(Spark::SceneDrawItem& item, const float x, const float y, const float z) noexcept {
    item.model = Spark::Matrix4::Translation({x, y, z});
}

}  // namespace

/**
 * CPU contract for the 3D transparency stack:
 *
 * 1. Opaque HDR pass (<c>params.draws</c>)
 * 2. Opaque snapshot when <c>waterDraws</c> OR <c>transparentDraws</c> non-empty (binding 13 color; depth for water)
 * 3. <c>waterDraws</c> (SSR / refraction use depth scratch)
 * 4. <c>transparentDraws</c> (alpha + transmission sample binding 13 — water must run before glass)
 *
 * These tests lock partition + sort behavior so pass-order refactors do not silently break transmission.
 */

TEST(TransparentPassPartitionTest, OpaqueSnapshotRequiredWhenWaterOrTransparent) {
    EXPECT_FALSE(Spark::VulkanSceneOpaqueSnapshot::IsRequired(false, false));
    EXPECT_TRUE(Spark::VulkanSceneOpaqueSnapshot::IsRequired(true, false));
    EXPECT_TRUE(Spark::VulkanSceneOpaqueSnapshot::IsRequired(false, true));
    EXPECT_TRUE(Spark::VulkanSceneOpaqueSnapshot::IsRequired(true, true));
}

TEST(TransparentPassPartitionTest, OpacityBelowOneRoutesToTransparentPass) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem faded{};
    faded.opacity = 0.5F;
    Spark::SceneDrawItem solid{};
    solid.opacity = 1.0F;

    items.PushBack(faded);
    items.PushBack(solid);

    Spark::SceneRenderParams params{};
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    ASSERT_EQ(params.draws.GetSize(), 1U);
    ASSERT_EQ(params.transparentDraws.GetSize(), 1U);
    EXPECT_FLOAT_EQ(params.draws[0].opacity, 1.0F);
    EXPECT_FLOAT_EQ(params.transparentDraws[0].opacity, 0.5F);
    EXPECT_EQ(params.transparentDraws[0].renderQueue, Spark::SceneRenderQueue::Transparent);
}

TEST(TransparentPassPartitionTest, OpacityEpsilonBoundary) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem atThreshold{};
    atThreshold.opacity = 0.999F;
    Spark::SceneDrawItem barelyFaded{};
    barelyFaded.opacity = 0.998F;

    items.PushBack(atThreshold);
    items.PushBack(barelyFaded);

    Spark::SceneRenderParams params{};
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    EXPECT_EQ(params.draws.GetSize(), 1U);
    EXPECT_EQ(params.transparentDraws.GetSize(), 1U);
    EXPECT_FLOAT_EQ(params.draws[0].opacity, 0.999F);
    EXPECT_FLOAT_EQ(params.transparentDraws[0].opacity, 0.998F);
}

TEST(TransparentPassPartitionTest, AlphaBlendRoutesDespiteFullOpacity) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem blend{};
    blend.opacity = 1.0F;
    blend.alphaBlend = true;

    items.PushBack(blend);

    Spark::SceneRenderParams params{};
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    ASSERT_EQ(params.draws.GetSize(), 0U);
    ASSERT_EQ(params.transparentDraws.GetSize(), 1U);
    EXPECT_TRUE(params.transparentDraws[0].alphaBlend);
}

TEST(TransparentPassPartitionTest, TransmissionRoutesDespiteFullOpacity) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem glass{};
    glass.opacity = 1.0F;
    glass.gltfExtensions.transmissionFactor = 0.35F;

    Spark::SceneDrawItem sheetBelowCutoff{};
    sheetBelowCutoff.opacity = 1.0F;
    sheetBelowCutoff.gltfExtensions.transmissionFactor = 0.01F;

    items.PushBack(glass);
    items.PushBack(sheetBelowCutoff);

    Spark::SceneRenderParams params{};
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    ASSERT_EQ(params.draws.GetSize(), 1U);
    ASSERT_EQ(params.transparentDraws.GetSize(), 1U);
    EXPECT_GT(params.transparentDraws[0].gltfExtensions.transmissionFactor, 0.01F);
    EXPECT_LE(params.draws[0].gltfExtensions.transmissionFactor, 0.01F);
}

TEST(TransparentPassPartitionTest, SkyDrawsStayInOpaqueList) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem sky{};
    sky.skyMode = Spark::SceneSkyMode::Dome;
    sky.opacity = 0.2F;
    sky.alphaBlend = true;

    items.PushBack(sky);

    Spark::SceneRenderParams params{};
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    ASSERT_EQ(params.draws.GetSize(), 1U);
    EXPECT_EQ(params.transparentDraws.GetSize(), 0U);
}

TEST(TransparentPassPartitionTest, OpaqueDrawOrderPreservedAcrossPartition) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem first{};
    first.mesh = Spark::SceneMeshSlot::GroundPlane;
    first.opacity = 1.0F;
    Spark::SceneDrawItem second{};
    second.mesh = Spark::SceneMeshSlot::UnitCube;
    second.opacity = 1.0F;
    Spark::SceneDrawItem third{};
    third.mesh = Spark::SceneMeshSlot::Custom;
    third.opacity = 0.5F;

    items.PushBack(second);
    items.PushBack(first);
    items.PushBack(third);

    Spark::SceneDrawListSorter::StableSortOpaqueDraws(items);

    Spark::SceneRenderParams params{};
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    ASSERT_EQ(params.draws.GetSize(), 2U);
    EXPECT_EQ(params.draws[0].mesh, Spark::SceneMeshSlot::GroundPlane);
    EXPECT_EQ(params.draws[1].mesh, Spark::SceneMeshSlot::UnitCube);
    ASSERT_EQ(params.transparentDraws.GetSize(), 1U);
    EXPECT_EQ(params.transparentDraws[0].mesh, Spark::SceneMeshSlot::Custom);
}

TEST(TransparentPassPartitionTest, BackToFrontSortUsesSquaredDistance) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem nearGlass{};
    nearGlass.opacity = 0.6F;
    SetTranslation(nearGlass, 0.0F, 0.0F, -3.0F);

    Spark::SceneDrawItem farGlass{};
    farGlass.opacity = 0.6F;
    SetTranslation(farGlass, 0.0F, 0.0F, -30.0F);

    items.PushBack(nearGlass);
    items.PushBack(farGlass);

    Spark::SceneRenderParams params{};
    params.transparentSortMode = Spark::SceneTransparentSortMode::BackToFrontByDepth;
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    ASSERT_EQ(params.transparentDraws.GetSize(), 2U);
    const float dist0 = SquaredDistanceFromCamera(params.transparentDraws[0], kCam);
    const float dist1 = SquaredDistanceFromCamera(params.transparentDraws[1], kCam);
    EXPECT_GT(dist0, dist1);
    EXPECT_NEAR(params.transparentDraws[0].model.m[14], -30.0F, 1.0e-5F);
    EXPECT_NEAR(params.transparentDraws[1].model.m[14], -3.0F, 1.0e-5F);
}

TEST(TransparentPassPartitionTest, TransparentSortNoneKeepsSubmissionOrder) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem first{};
    first.opacity = 0.5F;
    SetTranslation(first, 0.0F, 0.0F, -3.0F);
    Spark::SceneDrawItem second{};
    second.opacity = 0.5F;
    SetTranslation(second, 0.0F, 0.0F, -30.0F);

    items.PushBack(first);
    items.PushBack(second);

    Spark::SceneRenderParams params{};
    params.transparentSortMode = Spark::SceneTransparentSortMode::None;
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    ASSERT_EQ(params.transparentDraws.GetSize(), 2U);
    EXPECT_NEAR(params.transparentDraws[0].model.m[14], -3.0F, 1.0e-5F);
    EXPECT_NEAR(params.transparentDraws[1].model.m[14], -30.0F, 1.0e-5F);
}

TEST(TransparentPassPartitionTest, TransmissionAndFadeInSameTransparentList) {
    Spark::Array<Spark::SceneDrawItem> items;
    Spark::SceneDrawItem transmission{};
    transmission.opacity = 1.0F;
    transmission.gltfExtensions.transmissionFactor = 0.8F;
    SetTranslation(transmission, 4.0F, 0.0F, -10.0F);

    Spark::SceneDrawItem fade{};
    fade.opacity = 0.3F;
    SetTranslation(fade, -4.0F, 0.0F, -10.0F);

    items.PushBack(fade);
    items.PushBack(transmission);

    Spark::SceneRenderParams params{};
    params.transparentSortMode = Spark::SceneTransparentSortMode::BackToFrontByDepth;
    Spark::SceneDrawListSorter::PartitionIntoSceneParams(items, params, kCam);

    ASSERT_EQ(params.draws.GetSize(), 0U);
    ASSERT_EQ(params.transparentDraws.GetSize(), 2U);
    EXPECT_GT(params.transparentDraws[0].gltfExtensions.transmissionFactor, 0.01F);
    EXPECT_LT(params.transparentDraws[1].opacity, 0.999F);
    EXPECT_GT(
            SquaredDistanceFromCamera(params.transparentDraws[0], kCam),
            SquaredDistanceFromCamera(params.transparentDraws[1], kCam));
}