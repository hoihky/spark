#include "spark/audio/SoundClip.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/memory/SharedPtr.hpp"

#include <cstdlib>

extern "C" {
#define STB_VORBIS_IMPLEMENTATION
#include "stb_vorbis.c"
}

namespace Spark {

SharedPtr<SoundClip> TryLoadSoundClipFromOggFile(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return SharedPtr<SoundClip>();
    }
    short* pcm = nullptr;
    int channels = 0;
    int rate = 0;
    /** Per-channel sample count (time frames); interleaved shorts length is <c>frames * channels</c>. */
    const int frames = stb_vorbis_decode_filename(path, &channels, &rate, &pcm);
    if (frames <= 0 || pcm == nullptr || channels < 1 || rate <= 0) {
        if (pcm != nullptr) {
            std::free(pcm);
        }
        return SharedPtr<SoundClip>();
    }

    const std::size_t frameCount = static_cast<std::size_t>(frames);
    Array<float> interleaved;
    interleaved.Reserve(frameCount * 2U);

    for (std::size_t fi = 0; fi < frameCount; ++fi) {
        float leftCh = 0.0F;
        float rightCh = 0.0F;
        if (channels == 1) {
            const float mono = static_cast<float>(pcm[fi]) * (1.0F / 32768.0F);
            leftCh = mono;
            rightCh = mono;
        } else {
            const std::size_t base = fi * static_cast<std::size_t>(channels);
            leftCh = static_cast<float>(pcm[base]) * (1.0F / 32768.0F);
            rightCh = static_cast<float>(pcm[base + 1U]) * (1.0F / 32768.0F);
        }
        interleaved.PushBack(leftCh);
        interleaved.PushBack(rightCh);
    }
    std::free(pcm);

    auto* raw = new SoundClip(MoveTemp(interleaved), static_cast<std::uint32_t>(rate));
    return SharedPtr<SoundClip>(raw);
}

}  // namespace Spark
