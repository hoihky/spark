#pragma once

#include "spark/math/Vector3.hpp"

namespace Spark {

/** Resolved world-space listener used by <c>SoundMixer</c> for spatial panning/attenuation. */
struct AudioListenerPose {
    Vector3 position{0.0F, 0.0F, 0.0F};
    Vector3 forward{0.0F, 0.0F, -1.0F};
    Vector3 right{1.0F, 0.0F, 0.0F};
    Vector3 up{0.0F, 1.0F, 0.0F};
    bool valid = false;
};

/** Thread-local frame listener pose (written by <c>ProcessAudioListeners</c>). */
[[nodiscard]] AudioListenerPose& GetFrameAudioListenerPose() noexcept;
[[nodiscard]] const AudioListenerPose& GetFrameAudioListenerPoseConst() noexcept;

}  // namespace Spark
