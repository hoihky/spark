#pragma once

#include <cstdint>

#include "spark/audio/SoundClip.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

/** Built-in gameplay/UI procedural clips (cached, 48 kHz stereo). */
enum class ProceduralSoundPreset : std::uint8_t {
    UiClick = 0,
    UiBack = 1,
    Jump = 2,
    LandSoft = 3,
    LandHard = 4,
    Footstep = 5,
    Collectible = 6,
    Hurt = 7,
    AttackSwing = 8,
    ExplosionPop = 9,
    VictoryFanfare = 10,
    MenuWhoosh = 11,
};

class ProceduralSoundPresets final {
public:
    /** Returns a shared clip for the preset (generates once per process). */
    [[nodiscard]] static SharedPtr<SoundClip> Get(ProceduralSoundPreset preset) noexcept;
};

}  // namespace Spark
