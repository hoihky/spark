#include "spark/audio/ISoundOutput.hpp"

#include "spark/config.hpp"
#include "spark/core/Array.hpp"
#include "spark/core/Utility.hpp"
#include "spark/memory/UniquePtr.hpp"

#include <AudioToolbox/AudioQueue.h>
#include <cstring>
#include <pthread.h>

namespace Spark {

namespace {

constexpr std::size_t kRingCapFloats = 131072;  // ~1.36 s stereo @ 48 kHz
constexpr int kNumBuffers = 3;

class AppleSoundOutput final : public ISoundOutput {
public:
    AppleSoundOutput() noexcept { pthread_mutex_init(&mtx, nullptr); }

    ~AppleSoundOutput() override {
        Stop();
        pthread_mutex_destroy(&mtx);
    }

    bool Start(std::uint32_t sampleRate, std::uint32_t channels) override {
        Stop();
        if (channels != 2U) {
            return false;
        }
        sampleRate = sampleRate == 0 ? 48000 : sampleRate;
        ring.Clear();
        ring.Resize(kRingCapFloats);
        for (std::size_t i = 0; i < kRingCapFloats; ++i) {
            ring[i] = 0.0F;
        }
        capMask = kRingCapFloats - 1U;
        head = 0;
        size = 0;

        AudioStreamBasicDescription fmt{};
        fmt.mSampleRate = static_cast<Float64>(sampleRate);
        fmt.mFormatID = kAudioFormatLinearPCM;
        fmt.mFormatFlags = static_cast<UInt32>(kAudioFormatFlagIsFloat) | static_cast<UInt32>(kAudioFormatFlagsNativeEndian) |
                static_cast<UInt32>(kAudioFormatFlagIsPacked);
        fmt.mBytesPerPacket = 8;
        fmt.mFramesPerPacket = 1;
        fmt.mBytesPerFrame = 8;
        fmt.mChannelsPerFrame = 2;
        fmt.mBitsPerChannel = 32;

        if (AudioQueueNewOutput(&fmt, AudioCallback, this, nullptr, nullptr, 0, &queue) != noErr || queue == nullptr) {
            return false;
        }

        const UInt32 bufferBytes = 4096U * 8U;
        for (int i = 0; i < kNumBuffers; ++i) {
            if (AudioQueueAllocateBuffer(queue, bufferBytes, &buffers[i]) != noErr) {
                Stop();
                return false;
            }
            AudioCallback(this, queue, buffers[i]);
        }
        if (AudioQueueStart(queue, nullptr) != noErr) {
            Stop();
            return false;
        }
        return true;
    }

    void Stop() noexcept override {
        if (queue != nullptr) {
            AudioQueueStop(queue, true);
            AudioQueueDispose(queue, true);
            queue = nullptr;
            for (int i = 0; i < kNumBuffers; ++i) {
                buffers[i] = nullptr;
            }
        }
        pthread_mutex_lock(&mtx);
        size = 0;
        head = 0;
        pthread_mutex_unlock(&mtx);
    }

    void SubmitInterleavedFloat(const float* samples, const std::size_t frameCount) noexcept override {
        if (samples == nullptr || frameCount == 0) {
            return;
        }
        const std::size_t n = frameCount * 2U;
        pthread_mutex_lock(&mtx);
        while (size + n > kRingCapFloats) {
            head = (head + 2U) & capMask;
            size -= 2U;
        }
        for (std::size_t i = 0; i < n; ++i) {
            ring[(head + size + i) & capMask] = samples[i];
        }
        size += n;
        pthread_mutex_unlock(&mtx);
    }

private:
    static void AudioCallback(void* userData, AudioQueueRef /*aq*/, AudioQueueBufferRef buffer) {
        auto* self = static_cast<AppleSoundOutput*>(userData);
        auto* dst = static_cast<float*>(buffer->mAudioData);
        const UInt32 capBytes = buffer->mAudioDataBytesCapacity;
        const std::size_t maxFloats = static_cast<std::size_t>(capBytes) / sizeof(float);
        pthread_mutex_lock(&self->mtx);
        const std::size_t take = maxFloats < self->size ? maxFloats : self->size;
        for (std::size_t i = 0; i < take; ++i) {
            dst[i] = self->ring[(self->head + i) & self->capMask];
        }
        self->head = (self->head + take) & self->capMask;
        self->size -= take;
        pthread_mutex_unlock(&self->mtx);
        for (std::size_t i = take; i < maxFloats; ++i) {
            dst[i] = 0.0F;
        }
        buffer->mAudioDataByteSize = static_cast<UInt32>(maxFloats * sizeof(float));
        (void)AudioQueueEnqueueBuffer(self->queue, buffer, 0, nullptr);
    }

    pthread_mutex_t mtx{};
    Array<float> ring{};
    std::size_t capMask = 0;
    std::size_t head = 0;
    std::size_t size = 0;
    AudioQueueRef queue = nullptr;
    AudioQueueBufferRef buffers[kNumBuffers]{};
    std::uint32_t sampleRate = 48000;
};

}  // namespace

UniquePtr<ISoundOutput> CreatePlatformSoundOutput() {
    return UniquePtr<ISoundOutput>(MakeUnique<AppleSoundOutput>().Release());
}

}  // namespace Spark
