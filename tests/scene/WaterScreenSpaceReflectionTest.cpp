#include <gtest/gtest.h>

#include <cstddef>

#include "spark/ecs/components/water/WaterBodyComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/scene/VulkanWaterPushConstants.hpp"
#include "spark/render/scene/WaterPushConstantsBuilder.hpp"
#include "spark/scene/water/WaterRenderingProfile.hpp"
#include "spark/scene/water/WaterScreenSpaceReflectionSettings.hpp"

namespace {

Spark::ResolvedWaterScreenSpaceReflection ResolveDefault(const Spark::WaterScreenSpaceReflectionSettings& body) {
    return Spark::ResolveWaterScreenSpaceReflection(body, Spark::WaterRenderingProfile::Default);
}

}  // namespace

TEST(WaterScreenSpaceReflectionTest, DefaultSettingsAreEnabled) {
    const Spark::WaterScreenSpaceReflectionSettings settings{};
    EXPECT_TRUE(settings.IsEnabled());
    EXPECT_EQ(settings.GetMaxSteps(), Spark::WaterScreenSpaceReflectionSettings::kDefaultMaxSteps);
    EXPECT_FLOAT_EQ(settings.GetMaxRayDistance(), Spark::WaterScreenSpaceReflectionSettings::kDefaultMaxRayDistance);
}

TEST(WaterScreenSpaceReflectionTest, WaterBodyComponentExposesSsrSettings) {
    Spark::WaterBodyComponent water{};
    EXPECT_TRUE(water.GetSsrSettings().IsEnabled());
    water.SetSsrEnabled(false);
    EXPECT_FALSE(water.GetSsrSettings().IsEnabled());
}

TEST(WaterScreenSpaceReflectionTest, PushConstantsPackSsrFields) {
    Spark::SceneWaterDraw draw{};
    draw.ssrSettings.SetEnabled(true);
    draw.ssrSettings.SetMaxSteps(16);
    draw.ssrSettings.SetMaxRayDistance(32.0F);
    draw.ssrSettings.SetThickness(0.2F);
    draw.ssrSettings.SetStrength(0.75F);
    draw.resolvedSsr = ResolveDefault(draw.ssrSettings);

    const Spark::WaterPushConstants push = Spark::WaterPushConstantsBuilder(draw).Get();
    EXPECT_FLOAT_EQ(push.ssrEnabled, 1.0F);
    EXPECT_EQ(push.ssrMaxSteps, 16);
    EXPECT_FLOAT_EQ(push.ssrMaxRayDistance, 32.0F);
    EXPECT_FLOAT_EQ(push.ssrThickness, 0.2F);
    EXPECT_FLOAT_EQ(push.ssrStrength, 0.75F);
    EXPECT_FLOAT_EQ(push.ssrHalfRes, 0.0F);
}

TEST(WaterScreenSpaceReflectionTest, MinSpecProfileDisablesSsr) {
    Spark::WaterScreenSpaceReflectionSettings body{};
    body.SetEnabled(true);
    const Spark::ResolvedWaterScreenSpaceReflection resolved =
            Spark::ResolveWaterScreenSpaceReflection(body, Spark::WaterRenderingProfile::MinSpec);
    EXPECT_FALSE(resolved.enabled);
    EXPECT_EQ(resolved.maxSteps, 0);
}

TEST(WaterScreenSpaceReflectionTest, BalancedProfileHalfResAndCapsSteps) {
    Spark::WaterScreenSpaceReflectionSettings body{};
    body.SetMaxSteps(24);
    const Spark::ResolvedWaterScreenSpaceReflection resolved =
            Spark::ResolveWaterScreenSpaceReflection(body, Spark::WaterRenderingProfile::Balanced);
    EXPECT_TRUE(resolved.enabled);
    EXPECT_TRUE(resolved.halfRes);
    EXPECT_EQ(resolved.maxSteps, 12);
}

TEST(WaterScreenSpaceReflectionTest, GpuLayoutOffsets) {
    EXPECT_EQ(sizeof(Spark::WaterPushConstants), 316U);
    EXPECT_EQ(offsetof(Spark::WaterPushConstants, ssrEnabled), 280U);
    EXPECT_EQ(offsetof(Spark::WaterPushConstants, ssrMaxSteps), 296U);
    EXPECT_EQ(offsetof(Spark::WaterPushConstants, ssrHorizonFadeEnd), 308U);
    EXPECT_EQ(offsetof(Spark::WaterPushConstants, ssrHalfRes), 312U);
}
