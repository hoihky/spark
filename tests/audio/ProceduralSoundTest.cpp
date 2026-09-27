#include <gtest/gtest.h>

#include "spark/audio/ProceduralSoundPresets.hpp"
#include "spark/audio/SoundClip.hpp"

TEST(ProceduralSound, GeneratorsProduceAudio) {
    const Spark::SharedPtr<Spark::SoundClip> sweep = Spark::SoundClip::CreateToneSweep(200.0F, 800.0F, 0.05F, 0.2F);
    ASSERT_TRUE(sweep);
    EXPECT_GT(sweep->GetFrameCount(), 0U);

    const Spark::SharedPtr<Spark::SoundClip> noise = Spark::SoundClip::CreateNoiseBurst(0.04F, 0.18F, 1200.0F);
    ASSERT_TRUE(noise);
    EXPECT_GT(noise->GetFrameCount(), 0U);

    const Spark::SharedPtr<Spark::SoundClip> chime = Spark::SoundClip::CreateLayeredChime(440.0F, 660.0F, 0.08F, 0.2F);
    ASSERT_TRUE(chime);
    EXPECT_GT(chime->GetFrameCount(), 0U);
}

TEST(ProceduralSound, PresetsAreCached) {
    const Spark::SharedPtr<Spark::SoundClip> a = Spark::ProceduralSoundPresets::Get(Spark::ProceduralSoundPreset::Jump);
    const Spark::SharedPtr<Spark::SoundClip> b = Spark::ProceduralSoundPresets::Get(Spark::ProceduralSoundPreset::Jump);
    ASSERT_TRUE(a);
    EXPECT_EQ(a.Get(), b.Get());
}
