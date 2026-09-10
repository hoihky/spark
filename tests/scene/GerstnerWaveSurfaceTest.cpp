#include <gtest/gtest.h>

#include "spark/scene/water/GerstnerWaveSurface.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"

namespace {

Spark::WaterWaveSettings SingleWaveSettings() {
    Spark::WaterWaveSettings settings{};
    Spark::GerstnerWave& wave = settings.GetWave(0);
    wave.direction = {1.0F, 0.0F};
    wave.amplitude = 0.5F;
    wave.wavelength = 8.0F;
    wave.speed = 0.0F;
    wave.steepness = 0.25F;
    settings.SetActiveWaveCount(1);
    return settings;
}

}  // namespace

TEST(GerstnerWaveSurfaceTest, SingleWaveHeightAtOriginTimeZero) {
    const Spark::GerstnerWaveSurface surface(SingleWaveSettings());
    EXPECT_NEAR(surface.SampleHeight(0.0F, 0.0F, 0.0F), 0.5F, 1.0e-4F);
}

TEST(GerstnerWaveSurfaceTest, FlatSeaStateNormalPointsUp) {
    const Spark::GerstnerWaveSurface surface(SingleWaveSettings());
    const Spark::Vector3 normal = surface.SampleNormal(0.0F, 0.0F, 0.0F);
    EXPECT_NEAR(normal.y, 1.0F, 1.0e-3F);
}

TEST(GerstnerWaveSurfaceTest, PresetProducesActiveWaves) {
    const Spark::GerstnerWaveSurface surface =
            Spark::GerstnerWaveSurface::FromPreset(Spark::WaterWavePresetId::CalmLake);
    EXPECT_GE(surface.GetSettings().GetActiveWaveCount(), 1U);
    EXPECT_GT(surface.SampleHeight(1.0F, 2.0F, 0.5F), -1.0F);
}

TEST(GerstnerWaveSurfaceTest, HorizontalDisplacementIsZeroAtOriginTimeZero) {
    const Spark::GerstnerWaveSurface surface(SingleWaveSettings());
    const Spark::Vector2 disp = surface.SampleHorizontalDisplacement(0.0F, 0.0F, 0.0F);
    EXPECT_NEAR(disp.x, 0.0F, 1.0e-4F);
    EXPECT_NEAR(disp.y, 0.0F, 1.0e-4F);
}
