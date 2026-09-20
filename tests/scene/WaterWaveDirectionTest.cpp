#include <gtest/gtest.h>

#include "spark/scene/water/WaterWaveDirection.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"

TEST(WaterWaveDirectionTest, RotatesPrimarySwellToTarget) {
    Spark::WaterWaveSettings settings = Spark::WaterWaveSettings::FromPreset(Spark::WaterWavePresetId::CalmLake);
    ASSERT_GE(settings.GetActiveWaveCount(), 1U);

    const Spark::GerstnerWave& before = settings.GetWave(0);
    EXPECT_GT(before.direction.x, 0.9F);

    Spark::WaterWaveDirection::RotateSettingsToPrimarySwell(settings, {-1.0F, 0.0F});

    const Spark::GerstnerWave& after = settings.GetWave(0);
    EXPECT_LT(after.direction.x, -0.99F);
    EXPECT_NEAR(after.direction.y, 0.0F, 1.0e-3F);
}
