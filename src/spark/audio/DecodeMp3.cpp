#include "spark/audio/SoundClip.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/memory/SharedPtr.hpp"

#include <cstdio>
#include <cstdlib>

#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

namespace Spark {

SharedPtr<SoundClip> TryLoadSoundClipFromMp3File(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return SharedPtr<SoundClip>();
    }
    mp3dec_t dec{};
    mp3dec_init(&dec);
    mp3dec_file_info_t info{};
    if (mp3dec_load(&dec, path, &info, nullptr, nullptr) != 0) {
        return SharedPtr<SoundClip>();
    }
    if (info.buffer == nullptr || info.samples == 0U || info.channels < 1 || info.hz <= 0) {
        std::free(info.buffer);
        return SharedPtr<SoundClip>();
    }

    const int ch = info.channels;
    const std::size_t totalS16 = info.samples;
    const std::size_t frames = totalS16 / static_cast<std::size_t>(ch);

    Array<float> interleaved;
    interleaved.Reserve(frames * 2U);
    const auto* samples = info.buffer;
    for (std::size_t f = 0; f < frames; ++f) {
        float L = 0.0F;
        float R = 0.0F;
        if (ch == 1) {
            L = R = static_cast<float>(samples[f]) * (1.0F / 32768.0F);
        } else {
            const std::size_t b = f * static_cast<std::size_t>(ch);
            L = static_cast<float>(samples[b]) * (1.0F / 32768.0F);
            R = static_cast<float>(samples[b + 1U]) * (1.0F / 32768.0F);
        }
        interleaved.PushBack(L);
        interleaved.PushBack(R);
    }
    std::free(info.buffer);

    auto* raw = new SoundClip(MoveTemp(interleaved), static_cast<std::uint32_t>(info.hz));
    return SharedPtr<SoundClip>(raw);
}

}  // namespace Spark
