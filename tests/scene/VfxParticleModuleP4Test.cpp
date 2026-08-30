#include <gtest/gtest.h>

#include <cstring>

#include "spark/ecs/GameObject.hpp"
#include "spark/ecs/components/rendering/ParticleEmitterComponent.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/math/Vector4.hpp"
#include "spark/scene/core/GameWorld.hpp"
#include "spark/scene/serialization/ComponentSnapshotRegistry.hpp"
#include "spark/scene/serialization/IComponentSnapshotHandler.hpp"
#include "spark/scene/vfx/ParticleCurves.hpp"
#include "spark/scene/vfx/modules/ParticleModuleRegistry.hpp"

namespace {

TEST(VfxParticleCurveP4Test, FloatCurveInterpolatesEndpoints) {
    Spark::ParticleFloatCurve curve{};
    curve.SetEndpoints(2.0F, 0.0F);
    EXPECT_NEAR(curve.Evaluate(0.0F), 2.0F, 1.0e-5F);
    EXPECT_NEAR(curve.Evaluate(1.0F), 0.0F, 1.0e-5F);
    EXPECT_NEAR(curve.Evaluate(0.5F), 1.0F, 1.0e-5F);
}

TEST(VfxParticleCurveP4Test, ColorCurveInterpolatesEndpoints) {
    Spark::ParticleColorCurve curve{};
    curve.SetEndpoints({1.0F, 0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F, 0.0F});
    const Spark::Vector4 mid = curve.Evaluate(0.5F);
    EXPECT_NEAR(mid.x, 0.5F, 1.0e-5F);
    EXPECT_NEAR(mid.y, 0.5F, 1.0e-5F);
    EXPECT_NEAR(mid.w, 0.5F, 1.0e-5F);
}

TEST(VfxParticleModuleP4Test, RegistryResolvesBuiltinModules) {
    EXPECT_STREQ(Spark::ParticleModuleRegistry::Get("continuous").GetId(), "continuous");
    EXPECT_STREQ(Spark::ParticleModuleRegistry::Get("burst_only").GetId(), "burst_only");
    EXPECT_STREQ(Spark::ParticleModuleRegistry::Get("ring").GetId(), "ring");
    EXPECT_STREQ(Spark::ParticleModuleRegistry::Get("unknown").GetId(), "continuous");
}

TEST(VfxParticleModuleP4Test, EmitterUsesSizeColorCurvesOnCollect) {
    Spark::ParticleEmitterComponent emitter{};
    emitter.SetMaxParticles(4);
    emitter.SetLifetime(1.0F, 1.0F);
    emitter.SetStartEndSize(0.2F, 0.05F);

    Spark::ParticleFloatCurve sizeCurve{};
    sizeCurve.SetEndpoints(0.2F, 0.05F);
    emitter.SetSizeCurve(sizeCurve);

    Spark::ParticleColorCurve colorCurve{};
    colorCurve.SetEndpoints({1.0F, 1.0F, 1.0F, 1.0F}, {1.0F, 0.0F, 0.0F, 0.0F});
    emitter.SetColorCurve(colorCurve);

    Spark::Array<Spark::SceneParticleInstance> instances{};
    emitter.CollectInstances(instances);
    EXPECT_TRUE(instances.IsEmpty());
}

TEST(ParticleEmitterSnapshotTest, Legacy22FieldPayloadRestores) {
    Spark::GameWorld world{};
    Spark::GameObject* restored = world.CreateGameObject();
    const Spark::IComponentSnapshotHandler* handler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::ParticleEmitter);
    ASSERT_NE(handler, nullptr);

    Spark::ComponentRecord record{};
    record.kind = Spark::Utf8String("particle_emitter");
    record.payload = Spark::Utf8String(
            "1 256 24.000000 0.500000 1.000000 0.200000 0.050000 "
            "1.000000 0.500000 0.000000 1.000000 1.000000 0.000000 0.000000 0.000000 "
            "0.000000 -2.000000 0.000000 0.000000 1.000000 0.000000 0.400000 1.500000 3.000000");

    Spark::SceneApplyContext applyCtx{};
    ASSERT_TRUE(handler->TryRestore(*restored, record, world, applyCtx));

    const Spark::ParticleEmitterComponent* pe = restored->GetComponent<Spark::ParticleEmitterComponent>();
    ASSERT_NE(pe, nullptr);
    EXPECT_TRUE(pe->IsEmitterEnabled());
    EXPECT_EQ(pe->GetMaxParticles(), 256u);
    EXPECT_FLOAT_EQ(pe->GetEmissionRate(), 24.0F);
    EXPECT_FLOAT_EQ(pe->GetSpeedMax(), 3.0F);
    EXPECT_FALSE(pe->GetUseLocalEmission());
    EXPECT_STREQ(pe->GetEmissionModuleId(), "continuous");
}

TEST(ParticleEmitterSnapshotTest, P4FieldsRoundTrip) {
    Spark::GameWorld world{};
    Spark::GameObject* source = world.CreateGameObject();
    Spark::GameObject* restored = world.CreateGameObject();

    Spark::ParticleEmitterComponent* pe = source->AddComponent<Spark::ParticleEmitterComponent>();
    pe->SetEmitterEnabled(true);
    pe->SetMaxParticles(128);
    pe->SetEmissionRate(0.0F);
    pe->SetLifetime(0.3F, 0.6F);
    pe->SetStartEndSize(0.25F, 0.01F);
    pe->SetStartEndColor({1.0F, 0.8F, 0.2F, 1.0F}, {1.0F, 0.1F, 0.0F, 0.0F});
    pe->SetGravity({0.0F, -3.5F, 0.0F});
    pe->SetEmissionDirection({0.0F, 1.0F, 0.0F});
    pe->SetSpreadAngleRadians(0.75F);
    pe->SetSpeedRange(2.0F, 4.0F);
    pe->SetUseLocalEmission(true);
    pe->SetEmissionModuleId("ring");
    pe->SetRingRadius(0.9F);
    pe->SetUvRect({0.1F, 0.2F, 0.8F, 0.7F});

    Spark::ParticleFloatCurve sizeCurve{};
    sizeCurve.keyframes[0] = {0.0F, 0.25F};
    sizeCurve.keyframes[1] = {0.35F, 0.4F};
    sizeCurve.keyframes[2] = {1.0F, 0.01F};
    sizeCurve.keyframeCount = 3;
    pe->SetSizeCurve(sizeCurve);

    Spark::ParticleColorCurve colorCurve{};
    colorCurve.keyframes[0] = {0.0F, {1.0F, 1.0F, 1.0F, 1.0F}};
    colorCurve.keyframes[1] = {1.0F, {1.0F, 0.0F, 0.0F, 0.0F}};
    colorCurve.keyframeCount = 2;
    pe->SetColorCurve(colorCurve);

    const Spark::IComponentSnapshotHandler* handler =
            Spark::ComponentSnapshotRegistry::Default().Find(Spark::ComponentKind::ParticleEmitter);
    ASSERT_NE(handler, nullptr);

    Spark::SceneCaptureContext captureCtx{};
    Spark::ComponentRecord captured{};
    ASSERT_TRUE(handler->TryCapture(*source, captureCtx, captured));
    EXPECT_NE(std::strstr(captured.payload.CStr(), " p4 "), nullptr);

    Spark::SceneApplyContext applyCtx{};
    applyCtx.assetsRoot = "assets";
    applyCtx.assetLoader = &world.GetAssetLoader();
    ASSERT_TRUE(handler->TryRestore(*restored, captured, world, applyCtx));

    const Spark::ParticleEmitterComponent* restoredPe = restored->GetComponent<Spark::ParticleEmitterComponent>();
    ASSERT_NE(restoredPe, nullptr);
    EXPECT_TRUE(restoredPe->GetUseLocalEmission());
    EXPECT_STREQ(restoredPe->GetEmissionModuleId(), "ring");
    EXPECT_FLOAT_EQ(restoredPe->GetRingRadius(), 0.9F);
    EXPECT_FLOAT_EQ(restoredPe->GetUvRect().x, 0.1F);
    EXPECT_FLOAT_EQ(restoredPe->GetUvRect().w, 0.7F);
    EXPECT_EQ(restoredPe->GetSizeCurve().keyframeCount, 3u);
    EXPECT_NEAR(restoredPe->GetSizeCurve().keyframes[1].value, 0.4F, 1.0e-5F);
    EXPECT_EQ(restoredPe->GetColorCurve().keyframeCount, 2u);
    EXPECT_NEAR(restoredPe->GetColorCurve().keyframes[1].color.y, 0.0F, 1.0e-5F);
}

}  // namespace
