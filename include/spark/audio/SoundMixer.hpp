#pragma once

#include "spark/audio/SoundClip.hpp"
#include "spark/core/Array.hpp"
#include "spark/math/Vector3.hpp"
#include "spark/memory/SharedPtr.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/** Software voice pool + stereo interleaved mix (Open/Closed: new clip types plug via SoundClip). */
class SoundMixer {
public:
    void Configure(std::uint32_t outputSampleRate, std::size_t maxVoices) noexcept;

    /** Fire-and-forget; drops oldest voice if the pool is full. */
    void PlayOneShot(const SharedPtr<SoundClip>& clip, float volume) noexcept;

    /**
     * Spatial one-shot: pan/attenuate using the frame listener pose from <c>ProcessAudioListeners</c>.
     * <c>spatialBlend</c> 0 = centered mono, 1 = full stereo pan + distance falloff.
     */
    void PlayOneShotSpatial(
            const SharedPtr<SoundClip>& clip,
            float volume,
            const Vector3& worldPosition,
            float spatialBlend = 1.0F,
            float minDistance = 1.0F,
            float maxDistance = 48.0F) noexcept;

    /**
     * Continuous bed mixed under one-shots (separate voice; never evicted by <c>PlayOneShot</c>).
     * Typical use: looping <c>SoundClip</c> for background music while SFX use <c>PlayOneShot</c>.
     */
    void SetBackgroundMusic(const SharedPtr<SoundClip>& clip, float volume, bool loop) noexcept;

    void ClearBackgroundMusic() noexcept;

    /** Accumulate active voices into <c>outInterleavedStereo</c> (length <c>frameCount * 2</c> floats). */
    void MixAdd(float* outInterleavedStereo, std::size_t frameCount) noexcept;

    void StopAll() noexcept;

private:
    struct Voice {
        SharedPtr<SoundClip> clip{};
        double readIndex = 0.0;
        float volume = 1.0F;
        Vector3 worldPosition{0.0F, 0.0F, 0.0F};
        float spatialBlend = 0.0F;
        float minDistance = 1.0F;
        float maxDistance = 48.0F;
        bool active = false;
        bool loop = false;
    };

    void AccumulateVoiceForFrame(Voice& v, float& accL, float& accR) noexcept;

    Array<Voice> voices{};
    Voice music{};
    std::uint32_t outputRate = 48000;
};

}  // namespace Spark
