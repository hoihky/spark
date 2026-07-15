#pragma once

#include "spark/audio/ISoundOutput.hpp"
#include "spark/audio/SoundClip.hpp"
#include "spark/audio/SoundMixer.hpp"
#include "spark/core/Array.hpp"
#include "spark/memory/UniquePtr.hpp"

namespace Spark {

class VideoRecorder;

/**
 * Facade: owns mixer + platform output (Single Responsibility: games talk to SoundEngine only).
 * Call <c>PumpFrame</c> once per engine frame after simulation.
 */
class SoundEngine {
public:
    [[nodiscard]] bool Startup();
    void Shutdown() noexcept;

    void PumpFrame(float deltaTimeSeconds) noexcept;

    [[nodiscard]] SoundMixer& GetMixer() noexcept { return mixer; }
    [[nodiscard]] const SoundMixer& GetMixer() const noexcept { return mixer; }

    /** Looped or one-shot bed mixed under one-shots (see <c>SoundMixer::SetBackgroundMusic</c>). */
    void SetBackgroundMusic(const SharedPtr<SoundClip>& clip, float volume = 0.32F, bool loop = true) noexcept;

    void ClearBackgroundMusic() noexcept;

    /** When set and active, receives the same 48 kHz stereo mix sent to the device (for MP4 audio). */
    void SetVideoRecorder(VideoRecorder* recorder) noexcept { videoRecorder = recorder; }

    [[nodiscard]] bool IsRunning() const noexcept { return running; }

private:
    SoundMixer mixer{};
    UniquePtr<ISoundOutput> output{};
    Array<float> scratch{};
    VideoRecorder* videoRecorder = nullptr;
    bool running = false;
    static constexpr std::uint32_t kSampleRate = 48000;
    static constexpr std::uint32_t kChannels = 2;
};

}  // namespace Spark
