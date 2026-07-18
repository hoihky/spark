#pragma once

namespace Spark {

/** Frame ambient mix from <c>AmbientZoneComponent</c> volumes (listener-local). */
struct AmbientAudioMix {
    bool active = false;
    float volumeScale = 1.0F;
    float lowPassAmount = 0.0F;
};

[[nodiscard]] AmbientAudioMix& GetFrameAmbientAudioMix() noexcept;
[[nodiscard]] const AmbientAudioMix& GetFrameAmbientAudioMixConst() noexcept;

class GameWorld;

void ProcessAmbientZones(const GameWorld& world) noexcept;

}  // namespace Spark
