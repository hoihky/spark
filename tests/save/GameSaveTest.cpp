#include <gtest/gtest.h>

#include "spark/save/GameSave.hpp"

#include <cstdio>

TEST(GameSaveTest, RoundTripSlotFields) {
    Spark::GameSaveSlot slot{};
    slot.progress.activeLevelId = Spark::Utf8String("platformer");
    slot.progress.gemsCollected = 7;
    slot.progress.gemsTotal = 16;
    slot.progress.enemiesDefeated = 2;
    slot.progress.levelComplete = true;
    slot.settings.masterVolume = 0.8F;
    slot.settings.musicVolume = 0.25F;
    slot.settings.sfxVolume = 0.9F;

    const char* path = "/tmp/spark_game_save_test.savespark";
    ASSERT_TRUE(Spark::GameSave::TrySave(path, slot));

    Spark::GameSaveSlot loaded{};
    ASSERT_TRUE(Spark::GameSave::TryLoad(path, loaded));
    EXPECT_EQ(loaded.progress.gemsCollected, 7);
    EXPECT_EQ(loaded.progress.gemsTotal, 16);
    EXPECT_EQ(loaded.progress.enemiesDefeated, 2);
    EXPECT_TRUE(loaded.progress.levelComplete);
    EXPECT_STREQ(loaded.progress.activeLevelId.CStr(), "platformer");
    EXPECT_FLOAT_EQ(loaded.settings.musicVolume, 0.25F);
    std::remove(path);
}
