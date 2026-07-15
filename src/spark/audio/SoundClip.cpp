#include "spark/audio/SoundClip.hpp"

#include <cmath>

namespace Spark {

SharedPtr<SoundClip> SoundClip::CreateToneBlip(const float frequencyHz, const float durationSeconds, const float gain) {
    constexpr std::uint32_t kRate = 48000;
    const int n = static_cast<int>(durationSeconds * static_cast<float>(kRate));
    if (n <= 0) {
        return SharedPtr<SoundClip>();
    }
    Array<float> buf;
    buf.Reserve(static_cast<std::size_t>(n) * 2U);
    const float twoPiF = 6.2831855F * frequencyHz / static_cast<float>(kRate);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n > 1 ? n - 1 : 1);
        const float env = 0.5F * (1.0F - std::cos(3.14159265F * t));
        const float s = std::sin(twoPiF * static_cast<float>(i)) * gain * env;
        buf.PushBack(s);
        buf.PushBack(s);
    }
    auto* raw = new SoundClip(MoveTemp(buf), kRate);
    return SharedPtr<SoundClip>(raw);
}

SharedPtr<SoundClip> SoundClip::CreateSimpleAmbienceLoop() {
    constexpr std::uint32_t kRate = 48000;
    /** 96 Hz → 500 samples/period; 128 periods ≈ 2.67 s, all harmonics remain periodic in this length. */
    constexpr int kPeriodSamples = 500;
    constexpr int kNumPeriods = 128;
    const int n = kPeriodSamples * kNumPeriods;
    Array<float> buf;
    buf.Reserve(static_cast<std::size_t>(n) * 2U);
    constexpr float invSr = 1.0F / static_cast<float>(kRate);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) * invSr;
        const float w0 = 6.2831855F * 96.0F * t;
        const float w1 = 6.2831855F * 192.0F * t;
        const float w2 = 6.2831855F * 288.0F * t;
        const float s = 0.045F * std::sin(w0) + 0.028F * std::sin(w1) + 0.018F * std::sin(w2);
        buf.PushBack(s);
        buf.PushBack(s);
    }
    auto* raw = new SoundClip(MoveTemp(buf), kRate);
    return SharedPtr<SoundClip>(raw);
}

}  // namespace Spark
