#include <gtest/gtest.h>

#include "spark/ecs/components/rendering/MaterialComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/render/lighting/SceneLightingSetup.hpp"
#include "spark/render/lighting/SceneShadowParticipation.hpp"
#include "spark/scene/submit/LitSceneSubmitOptions.hpp"

namespace {

TEST(EngineApiFacadeTest, SceneShadowParticipationResolvesAgainstDefault) {
    const Spark::SceneShadowParticipation receiveOnly = Spark::SceneShadowParticipation::ReceiveOnly();
    const std::int32_t flags = receiveOnly.ResolveAgainstDefault(Spark::kSceneShadowCastAndReceive);
    EXPECT_EQ(flags & Spark::kSceneShadowCast, 0);
    EXPECT_EQ(flags & Spark::kSceneShadowReceive, Spark::kSceneShadowReceive);
}

TEST(EngineApiFacadeTest, MaterialShadowParticipationRoundTrip) {
    Spark::MaterialComponent material{};
    Spark::SceneShadowParticipation::CastOnly().ApplyTo(material);
    const Spark::SceneShadowParticipation read = material.GetShadowParticipation();
    EXPECT_EQ(read.GetCastOverride(), true);
    EXPECT_EQ(read.GetReceiveOverride(), false);
}

TEST(EngineApiFacadeTest, SceneLightingSetupAppliesProfileOverrides) {
    Spark::SceneRenderParams params{};
    Spark::SceneLightingSetup::FromProfile(Spark::SceneLightingProfile::Outdoor)
            .WithShadowCascades(0.2F, 900.0F)
            .WithShadowDistanceFade(180.0F)
            .WithDirectionalShadows(true)
            .ApplyTo(params);

    EXPECT_EQ(params.lightingProfile, Spark::SceneLightingProfile::Outdoor);
    EXPECT_FLOAT_EQ(params.shadowCascadeNear, 0.2F);
    EXPECT_FLOAT_EQ(params.shadowCascadeFar, 900.0F);
    EXPECT_FLOAT_EQ(params.shadowDistanceMax, 180.0F);
    EXPECT_TRUE(params.directionalShadowsEnabled);
}

TEST(EngineApiFacadeTest, SceneRenderParamsSanitizeClampsInvalidValues) {
    Spark::SceneRenderParams params{};
    params.lightIntensity = -2.0F;
    params.shadowBias = 1.0F;
    params.timeOfDay = 1.75F;
    params.ssaoStrength = 9.0F;
    params.Sanitize();

    EXPECT_FLOAT_EQ(params.lightIntensity, 0.0F);
    EXPECT_LT(params.shadowBias, 0.06F);
    EXPECT_GE(params.timeOfDay, 0.0F);
    EXPECT_LT(params.timeOfDay, 1.0F);
    EXPECT_LE(params.ssaoStrength, 2.0F);
}

TEST(EngineApiFacadeTest, LitSceneSubmitOptionsFluentApi) {
    Spark::LitSceneSubmitOptions options{};
    options.WithDirectionalLight({0.0F, -1.0F, 0.0F}, {1.0F, 0.9F, 0.8F}, 1.2F).WithAmbient({0.1F, 0.1F, 0.12F});

    EXPECT_FLOAT_EQ(options.lightIntensity, 1.2F);
    EXPECT_FLOAT_EQ(options.ambientColor.y, 0.1F);
}

}  // namespace
