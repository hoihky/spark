#include <gtest/gtest.h>

#include "spark/config.hpp"
#include "spark/scene/water/GerstnerWaveSurface.hpp"
#include "spark/scene/water/WaterPresetAssetLoader.hpp"
#include "spark/scene/water/WaterWaveSettings.hpp"

TEST(WaterPresetAssetLoaderTest, LoadsCalmLakeAssetFromDisk) {
    const Spark::AssetLoadOutcome<Spark::WaterPresetAsset> outcome =
            Spark::WaterPresetAssetLoader::TryLoadFromFile("calm_lake");
    ASSERT_TRUE(outcome.ok);
    EXPECT_EQ(outcome.value.presetId, Spark::WaterWavePresetId::CalmLake);
    EXPECT_EQ(outcome.value.settings.GetActiveWaveCount(), 3U);
    EXPECT_FLOAT_EQ(outcome.value.settings.GetWave(0).amplitude, 0.045F);
}

TEST(WaterPresetAssetLoaderTest, PresetLoaderMatchesFromSettingsFactory) {
    const Spark::WaterWaveSettings fromAsset =
            Spark::WaterPresetAssetLoader::TryLoadPreset(Spark::WaterWavePresetId::StormySea).value.settings;
    const Spark::WaterWaveSettings fromFactory =
            Spark::WaterWaveSettings::FromPreset(Spark::WaterWavePresetId::StormySea);
    EXPECT_EQ(fromAsset.GetActiveWaveCount(), fromFactory.GetActiveWaveCount());
    EXPECT_FLOAT_EQ(fromAsset.GetWave(0).wavelength, fromFactory.GetWave(0).wavelength);
    EXPECT_FLOAT_EQ(fromAsset.GetWave(3).steepness, fromFactory.GetWave(3).steepness);
}

TEST(WaterPresetAssetLoaderTest, GlobalWindSpeedScalesWaveSpeed) {
    const Spark::WaterWaveSettings calm = Spark::WaterWaveSettings::FromPreset(Spark::WaterWavePresetId::CalmLake, 2.0F);
    EXPECT_FLOAT_EQ(calm.GetGlobalWindSpeed(), 2.0F);
    EXPECT_FLOAT_EQ(calm.GetWave(0).speed, 1.10F);
}
