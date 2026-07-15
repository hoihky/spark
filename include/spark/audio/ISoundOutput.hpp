#pragma once

#include "spark/memory/UniquePtr.hpp"

#include <cstddef>
#include <cstdint>

namespace Spark {

/**
 * Low-level streaming output (Dependency Inversion): the mixer pushes interleaved float PCM;
 * implementations feed hardware or discard (Null / stub builds).
 *
 * Platform TUs: <c>SoundOutputApple.cpp</c> (AudioQueue), <c>SoundOutputWin32.cpp</c> (WASAPI shared mode,
 * 48 kHz stereo IEEE float mix format), <c>SoundOutputNull.cpp</c> (other targets). Exactly one defines
 * <c>CreatePlatformSoundOutput()</c> per configuration.
 */
class ISoundOutput {
public:
    virtual ~ISoundOutput() = default;

    /** Stereo interleaved frames: <c>frameCount</c> pairs => <c>2 * frameCount</c> floats. */
    virtual bool Start(std::uint32_t sampleRate, std::uint32_t channels) = 0;
    virtual void Stop() noexcept = 0;
    virtual void SubmitInterleavedFloat(const float* samples, std::size_t frameCount) noexcept = 0;
};

[[nodiscard]] UniquePtr<ISoundOutput> CreatePlatformSoundOutput();

}  // namespace Spark
