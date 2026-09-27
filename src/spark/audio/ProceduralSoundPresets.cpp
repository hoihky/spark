#include "spark/audio/ProceduralSoundPresets.hpp"

namespace Spark {

SharedPtr<SoundClip> ProceduralSoundPresets::Get(const ProceduralSoundPreset preset) noexcept {
    static SharedPtr<SoundClip> clips[12]{};
    const std::size_t index = static_cast<std::size_t>(preset);
    if (index >= 12U) {
        return SharedPtr<SoundClip>();
    }
    if (!clips[index]) {
        switch (preset) {
        case ProceduralSoundPreset::UiClick:
            clips[index] = SoundClip::CreateToneBlip(720.0F, 0.032F, 0.18F);
            break;
        case ProceduralSoundPreset::UiBack:
            clips[index] = SoundClip::CreateToneSweep(520.0F, 320.0F, 0.055F, 0.16F);
            break;
        case ProceduralSoundPreset::Jump:
            clips[index] = SoundClip::CreateToneSweep(280.0F, 520.0F, 0.085F, 0.22F);
            break;
        case ProceduralSoundPreset::LandSoft:
            clips[index] = SoundClip::CreateNoiseBurst(0.06F, 0.14F, 900.0F);
            break;
        case ProceduralSoundPreset::LandHard:
            clips[index] = SoundClip::CreateNoiseBurst(0.09F, 0.22F, 600.0F);
            break;
        case ProceduralSoundPreset::Footstep:
            clips[index] = SoundClip::CreateNoiseBurst(0.035F, 0.09F, 1400.0F);
            break;
        case ProceduralSoundPreset::Collectible:
            clips[index] = SoundClip::CreateLayeredChime(880.0F, 1320.0F, 0.11F, 0.2F);
            break;
        case ProceduralSoundPreset::Hurt:
            clips[index] = SoundClip::CreateToneSweep(420.0F, 180.0F, 0.12F, 0.24F);
            break;
        case ProceduralSoundPreset::AttackSwing:
            clips[index] = SoundClip::CreateNoiseBurst(0.05F, 0.12F, 2200.0F);
            break;
        case ProceduralSoundPreset::ExplosionPop:
            clips[index] = SoundClip::CreateNoiseBurst(0.14F, 0.28F, 400.0F);
            break;
        case ProceduralSoundPreset::VictoryFanfare:
            clips[index] = SoundClip::CreateLayeredChime(523.0F, 784.0F, 0.35F, 0.26F);
            break;
        case ProceduralSoundPreset::MenuWhoosh:
            clips[index] = SoundClip::CreateToneSweep(180.0F, 640.0F, 0.08F, 0.15F);
            break;
        }
    }
    return clips[index];
}

}  // namespace Spark
