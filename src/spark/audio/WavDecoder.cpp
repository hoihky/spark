#include "spark/audio/WavDecoder.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace Spark {

namespace {

[[nodiscard]] bool ReadU32(std::FILE* f, std::uint32_t& out) noexcept {
    unsigned char b[4];
    if (std::fread(b, 1, 4, f) != 4) {
        return false;
    }
    out = static_cast<std::uint32_t>(b[0]) | (static_cast<std::uint32_t>(b[1]) << 8U) |
            (static_cast<std::uint32_t>(b[2]) << 16U) | (static_cast<std::uint32_t>(b[3]) << 24U);
    return true;
}

[[nodiscard]] bool ReadU16(std::FILE* f, std::uint16_t& out) noexcept {
    unsigned char b[2];
    if (std::fread(b, 1, 2, f) != 2) {
        return false;
    }
    out = static_cast<std::uint16_t>(b[0]) | (static_cast<std::uint16_t>(b[1]) << 8U);
    return true;
}

}  // namespace

SharedPtr<SoundClip> TryLoadSoundClipFromWavFile(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return SharedPtr<SoundClip>();
    }
    std::FILE* f = std::fopen(path, "rb");
    if (f == nullptr) {
        return SharedPtr<SoundClip>();
    }
    char riff[4];
    std::uint32_t riffSize = 0;
    char wave[4];
    if (std::fread(riff, 1, 4, f) != 4 || std::memcmp(riff, "RIFF", 4) != 0 || !ReadU32(f, riffSize) ||
            std::fread(wave, 1, 4, f) != 4 || std::memcmp(wave, "WAVE", 4) != 0) {
        std::fclose(f);
        return SharedPtr<SoundClip>();
    }
    std::uint16_t audioFormat = 0;
    std::uint16_t numChannels = 0;
    std::uint32_t sampleRate = 0;
    std::uint16_t bitsPerSample = 0;
    Array<std::uint8_t> dataChunk;
    bool haveFmt = false;

    while (true) {
        char id[4];
        std::uint32_t sz = 0;
        if (std::fread(id, 1, 4, f) != 4 || !ReadU32(f, sz)) {
            break;
        }
        if (std::memcmp(id, "fmt ", 4) == 0) {
            if (!ReadU16(f, audioFormat) || !ReadU16(f, numChannels) || !ReadU32(f, sampleRate)) {
                std::fclose(f);
                return SharedPtr<SoundClip>();
            }
            std::uint32_t byteRate = 0;
            std::uint16_t blockAlign = 0;
            (void)ReadU32(f, byteRate);
            (void)ReadU16(f, blockAlign);
            if (!ReadU16(f, bitsPerSample)) {
                std::fclose(f);
                return SharedPtr<SoundClip>();
            }
            const std::uint32_t skip = sz > 16U ? sz - 16U : 0U;
            if (skip > 0U) {
                if (std::fseek(f, static_cast<long>(skip), SEEK_CUR) != 0) {
                    std::fclose(f);
                    return SharedPtr<SoundClip>();
                }
            }
            haveFmt = true;
        } else if (std::memcmp(id, "data", 4) == 0) {
            dataChunk.Clear();
            dataChunk.Reserve(sz);
            for (std::uint32_t i = 0; i < sz; ++i) {
                int c = std::fgetc(f);
                if (c < 0) {
                    std::fclose(f);
                    return SharedPtr<SoundClip>();
                }
                dataChunk.PushBack(static_cast<std::uint8_t>(c));
            }
            break;
        } else {
            if (std::fseek(f, static_cast<long>(sz), SEEK_CUR) != 0) {
                break;
            }
        }
        if ((sz & 1U) != 0U) {
            (void)std::fgetc(f);
        }
    }
    std::fclose(f);

    if (!haveFmt || dataChunk.IsEmpty() || audioFormat != 1 || bitsPerSample != 16 ||
            (numChannels != 1 && numChannels != 2)) {
        return SharedPtr<SoundClip>();
    }
    const std::size_t dataBytes = dataChunk.GetSize();
    const std::size_t sampleCount = dataBytes / sizeof(std::int16_t) / numChannels;
    if (sampleCount == 0) {
        return SharedPtr<SoundClip>();
    }

    Array<float> stereo;
    stereo.Reserve(sampleCount * 2U);
    const float inv = 1.0F / 32768.0F;
    const std::uint8_t* p = dataChunk.GetData();
    if (numChannels == 1) {
        for (std::size_t i = 0; i < sampleCount; ++i) {
            std::int16_t v = 0;
            std::memcpy(&v, p + i * sizeof(std::int16_t), sizeof(v));
            const float s = static_cast<float>(v) * inv;
            stereo.PushBack(s);
            stereo.PushBack(s);
        }
    } else {
        for (std::size_t i = 0; i < sampleCount; ++i) {
            std::int16_t lv = 0;
            std::int16_t rv = 0;
            std::memcpy(&lv, p + i * 2U * sizeof(std::int16_t), sizeof(lv));
            std::memcpy(&rv, p + i * 2U * sizeof(std::int16_t) + sizeof(lv), sizeof(rv));
            stereo.PushBack(static_cast<float>(lv) * inv);
            stereo.PushBack(static_cast<float>(rv) * inv);
        }
    }
    auto* clip = new SoundClip(MoveTemp(stereo), sampleRate);
    return SharedPtr<SoundClip>(clip);
}

}  // namespace Spark
