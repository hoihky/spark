#pragma once

#include <cstdint>

#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/memory/SharedPtr.hpp"

namespace Spark {

/** Decoded PCM as stereo interleaved 32-bit float, sample-rate tagged (used by mixer + WAV loader). */
class SoundClip {
public:
    SoundClip() = default;
    SoundClip(Array<float> interleavedStereo, std::uint32_t sampleRate) noexcept
            : samples(MoveTemp(interleavedStereo)), sampleRate(sampleRate) {}

    [[nodiscard]] const Array<float>& GetInterleavedStereo() const noexcept { return samples; }
    [[nodiscard]] std::uint32_t GetSampleRate() const noexcept { return sampleRate; }
    /** Number of stereo frames (pairs L/R). */
    [[nodiscard]] std::size_t GetFrameCount() const noexcept { return samples.GetSize() / 2U; }

    /** Short sine blip for UI / gameplay when no asset is present (48 kHz stereo). */
    [[nodiscard]] static SharedPtr<SoundClip> CreateToneBlip(float frequencyHz, float durationSeconds, float gain);

    /** Exponential-decay sine with optional pitch sweep (start → end Hz). */
    [[nodiscard]] static SharedPtr<SoundClip> CreateToneSweep(
            float startHz,
            float endHz,
            float durationSeconds,
            float gain);

    /** Band-limited noise burst with fast attack and exponential decay. */
    [[nodiscard]] static SharedPtr<SoundClip> CreateNoiseBurst(float durationSeconds, float gain, float lowPassHz = 0.0F);

    /** Two sine partials with a shared envelope (coin / pickup chime). */
    [[nodiscard]] static SharedPtr<SoundClip> CreateLayeredChime(
            float frequencyHzA,
            float frequencyHzB,
            float durationSeconds,
            float gain);

    /**
     * Soft looping pad (48 kHz stereo): exact harmonic periods so the buffer loops without a click.
     * Intended as placeholder background music when no authored track is loaded.
     */
    [[nodiscard]] static SharedPtr<SoundClip> CreateSimpleAmbienceLoop();

private:
    Array<float> samples;
    std::uint32_t sampleRate = 48000;
};

}  // namespace Spark
