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

SharedPtr<SoundClip> SoundClip::CreateToneSweep(
        const float startHz,
        const float endHz,
        const float durationSeconds,
        const float gain) {
    constexpr std::uint32_t kRate = 48000;
    const int n = static_cast<int>(durationSeconds * static_cast<float>(kRate));
    if (n <= 0) {
        return SharedPtr<SoundClip>();
    }
    Array<float> buf;
    buf.Reserve(static_cast<std::size_t>(n) * 2U);
    float phase = 0.0F;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n > 1 ? n - 1 : 1);
        const float env = std::exp(-4.5F * t);
        const float hz = startHz + (endHz - startHz) * t;
        phase += 6.2831855F * hz / static_cast<float>(kRate);
        const float s = std::sin(phase) * gain * env;
        buf.PushBack(s);
        buf.PushBack(s);
    }
    return SharedPtr<SoundClip>(new SoundClip(MoveTemp(buf), kRate));
}

SharedPtr<SoundClip> SoundClip::CreateNoiseBurst(
        const float durationSeconds,
        const float gain,
        const float lowPassHz) {
    constexpr std::uint32_t kRate = 48000;
    const int n = static_cast<int>(durationSeconds * static_cast<float>(kRate));
    if (n <= 0) {
        return SharedPtr<SoundClip>();
    }
    Array<float> buf;
    buf.Reserve(static_cast<std::size_t>(n) * 2U);
    std::uint32_t rng = 0x12345678U;
    float filterState = 0.0F;
    const float alpha = lowPassHz > 1.0F ? 1.0F - std::exp(-6.2831855F * lowPassHz / static_cast<float>(kRate)) : 1.0F;
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n > 1 ? n - 1 : 1);
        const float env = std::exp(-7.0F * t) * (1.0F - t);
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        const float white = (static_cast<float>(rng & 0xffffu) / 32768.0F) - 1.0F;
        filterState += alpha * (white - filterState);
        const float s = filterState * gain * env;
        buf.PushBack(s);
        buf.PushBack(s);
    }
    return SharedPtr<SoundClip>(new SoundClip(MoveTemp(buf), kRate));
}

SharedPtr<SoundClip> SoundClip::CreateLayeredChime(
        const float frequencyHzA,
        const float frequencyHzB,
        const float durationSeconds,
        const float gain) {
    constexpr std::uint32_t kRate = 48000;
    const int n = static_cast<int>(durationSeconds * static_cast<float>(kRate));
    if (n <= 0) {
        return SharedPtr<SoundClip>();
    }
    Array<float> buf;
    buf.Reserve(static_cast<std::size_t>(n) * 2U);
    const float wA = 6.2831855F * frequencyHzA / static_cast<float>(kRate);
    const float wB = 6.2831855F * frequencyHzB / static_cast<float>(kRate);
    for (int i = 0; i < n; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(n > 1 ? n - 1 : 1);
        const float env = 0.5F * (1.0F - std::cos(3.14159265F * t)) * std::exp(-2.8F * t);
        const float s = (0.65F * std::sin(wA * static_cast<float>(i)) + 0.35F * std::sin(wB * static_cast<float>(i))) *
                gain * env;
        buf.PushBack(s);
        buf.PushBack(s);
    }
    return SharedPtr<SoundClip>(new SoundClip(MoveTemp(buf), kRate));
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
