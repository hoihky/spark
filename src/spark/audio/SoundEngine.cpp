#include "spark/audio/SoundEngine.hpp"
#include "spark/audio/ISoundOutput.hpp"
#include "spark/media/VideoRecorder.hpp"

#include "spark/core/Utility.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <cmath>

namespace Spark {

namespace {

class NullSoundOutput final : public ISoundOutput {
public:
    bool Start(std::uint32_t /*sampleRate*/, std::uint32_t /*channels*/) override { return true; }

    void Stop() noexcept override {}

    void SubmitInterleavedFloat(const float* /*samples*/, std::size_t /*frameCount*/) noexcept override {}
};

}  // namespace

void SoundEngine::SetBackgroundMusic(const SharedPtr<SoundClip>& clip, const float volume, const bool loop) noexcept {
    if (!running) {
        return;
    }
    mixer.SetBackgroundMusic(clip, volume, loop);
}

void SoundEngine::ClearBackgroundMusic() noexcept {
    if (!running) {
        return;
    }
    mixer.ClearBackgroundMusic();
}

bool SoundEngine::Startup() {
    if (running) {
        return true;
    }
    output = CreatePlatformSoundOutput();
    if (!output) {
        output.Reset(MakeUnique<NullSoundOutput>().Release());
    }
    if (!output->Start(kSampleRate, kChannels)) {
        output.Reset();
        output.Reset(MakeUnique<NullSoundOutput>().Release());
        if (!output->Start(kSampleRate, kChannels)) {
            return false;
        }
    }
    mixer.Configure(kSampleRate, 48);
    scratch.Clear();
    scratch.Reserve(8192);
    running = true;
    return true;
}

void SoundEngine::Shutdown() noexcept {
    if (!running) {
        return;
    }
    mixer.StopAll();
    if (output) {
        output->Stop();
    }
    output.Reset();
    scratch.Clear();
    running = false;
}

void SoundEngine::PumpFrame(const float deltaTimeSeconds) noexcept {
    if (!running || !output) {
        return;
    }
    const float dt = deltaTimeSeconds > 0.0F ? deltaTimeSeconds : (1.0F / 60.0F);
    std::size_t frames = static_cast<std::size_t>(static_cast<double>(kSampleRate) * static_cast<double>(dt) + 0.5);
    if (frames < 1U) {
        frames = 1U;
    }
    if (frames > 4096U) {
        frames = 4096U;
    }
    const std::size_t floats = frames * 2U;
    scratch.Clear();
    for (std::size_t i = 0; i < floats; ++i) {
        scratch.PushBack(0.0F);
    }
    mixer.MixAdd(scratch.GetData(), frames);
    if (videoRecorder != nullptr && videoRecorder->IsActive()) {
        videoRecorder->AppendAudioInterleavedFloat(scratch.GetData(), frames);
    }
    output->SubmitInterleavedFloat(scratch.GetData(), frames);
}

}  // namespace Spark
