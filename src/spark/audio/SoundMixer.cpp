#include "spark/audio/SoundMixer.hpp"

#include "spark/audio/AmbientAudio.hpp"
#include "spark/audio/AudioListenerPose.hpp"

#include <algorithm>
#include <cmath>

namespace Spark {

void SoundMixer::Configure(const std::uint32_t outputSampleRate, const std::size_t maxVoices) noexcept {
    outputRate = outputSampleRate == 0 ? 48000 : outputSampleRate;
    voices.Clear();
    voices.Reserve(maxVoices);
    for (std::size_t i = 0; i < maxVoices; ++i) {
        voices.PushBack(Voice{});
    }
    ClearBackgroundMusic();
}

void SoundMixer::SetBackgroundMusic(const SharedPtr<SoundClip>& clip, const float volume, const bool loop) noexcept {
    if (!clip || clip->GetFrameCount() == 0) {
        ClearBackgroundMusic();
        return;
    }
    music.clip = clip;
    music.volume = volume;
    music.readIndex = 0.0;
    music.loop = loop;
    music.active = true;
}

void SoundMixer::ClearBackgroundMusic() noexcept {
    music.active = false;
    music.clip.Reset();
    music.readIndex = 0.0;
    music.volume = 1.0F;
    music.loop = false;
}

void SoundMixer::PlayOneShot(const SharedPtr<SoundClip>& clip, const float volume) noexcept {
    PlayOneShotSpatial(clip, volume, Vector3::Zero, 0.0F);
}

void SoundMixer::PlayOneShotSpatial(
        const SharedPtr<SoundClip>& clip,
        const float volume,
        const Vector3& worldPosition,
        const float spatialBlend,
        const float minDistance,
        const float maxDistance) noexcept {
    if (!clip || clip->GetFrameCount() == 0) {
        return;
    }
    std::size_t slot = voices.GetSize();
    for (std::size_t i = 0; i < voices.GetSize(); ++i) {
        if (!voices[i].active) {
            slot = i;
            break;
        }
    }
    if (slot >= voices.GetSize()) {
        std::size_t oldest = 0;
        for (std::size_t i = 1; i < voices.GetSize(); ++i) {
            if (voices[i].readIndex < voices[oldest].readIndex) {
                oldest = i;
            }
        }
        slot = oldest;
    }
    Voice& v = voices[slot];
    v.clip = clip;
    v.readIndex = 0.0;
    v.volume = volume;
    v.worldPosition = worldPosition;
    v.spatialBlend = spatialBlend;
    v.minDistance = (minDistance > 1.0e-4F) ? minDistance : 1.0F;
    v.maxDistance = (maxDistance > v.minDistance) ? maxDistance : v.minDistance + 1.0F;
    v.loop = false;
    v.active = true;
}

void SoundMixer::AccumulateVoiceForFrame(Voice& v, float& accL, float& accR) noexcept {
    if (!v.active || !v.clip) {
        return;
    }
    const std::uint32_t clipRate = v.clip->GetSampleRate() == 0 ? outputRate : v.clip->GetSampleRate();
    const double step = static_cast<double>(clipRate) / static_cast<double>(outputRate);
    const std::size_t maxFrame = v.clip->GetFrameCount();
    if (maxFrame == 0) {
        v.active = false;
        return;
    }
    if (!v.loop && v.readIndex >= static_cast<double>(maxFrame - 1U)) {
        v.active = false;
        return;
    }
    const double idx = v.readIndex;
    const std::size_t i0 = static_cast<std::size_t>(idx);
    const float t = static_cast<float>(idx - static_cast<double>(i0));
    const std::size_t i1 = i0 + 1U < maxFrame ? i0 + 1U : i0;
    const float* s = v.clip->GetInterleavedStereo().GetData();
    const float l0 = s[i0 * 2U];
    const float r0 = s[i0 * 2U + 1U];
    const float l1 = s[i1 * 2U];
    const float r1 = s[i1 * 2U + 1U];
    const float l = l0 + (l1 - l0) * t;
    const float r = r0 + (r1 - r0) * t;
    float gainL = 1.0F;
    float gainR = 1.0F;
    const AmbientAudioMix& ambient = GetFrameAmbientAudioMixConst();
    const float ambientVolume = ambient.active ? ambient.volumeScale : 1.0F;
    if (v.spatialBlend > 1.0e-4F) {
        const AudioListenerPose& listener = GetFrameAudioListenerPoseConst();
        if (listener.valid) {
            const Vector3 delta = v.worldPosition - listener.position;
            const float dist = std::sqrt(delta.LengthSquared());
            const float falloff = 1.0F - std::clamp(
                    (dist - v.minDistance) / (v.maxDistance - v.minDistance),
                    0.0F,
                    1.0F);
            const float pan = std::clamp(
                    Vector3::Dot(delta, listener.right) / std::max(dist, 1.0e-4F),
                    -1.0F,
                    1.0F);
            const float center = 0.5F * (1.0F - std::abs(pan));
            const float side = 0.5F * (1.0F + std::abs(pan));
            gainL = center + (pan < 0.0F ? side : 0.0F);
            gainR = center + (pan > 0.0F ? side : 0.0F);
            const float spatialGain = falloff * v.spatialBlend + (1.0F - v.spatialBlend);
            gainL *= spatialGain;
            gainR *= spatialGain;
        }
    }
    accL += l * v.volume * gainL * ambientVolume;
    accR += r * v.volume * gainR * ambientVolume;
    v.readIndex += step;
    if (v.loop && maxFrame > 0) {
        while (v.readIndex >= static_cast<double>(maxFrame)) {
            v.readIndex -= static_cast<double>(maxFrame);
        }
    } else if (v.readIndex >= static_cast<double>(maxFrame)) {
        v.active = false;
    }
}

void SoundMixer::MixAdd(float* outInterleavedStereo, const std::size_t frameCount) noexcept {
    if (outInterleavedStereo == nullptr || frameCount == 0) {
        return;
    }
    for (std::size_t f = 0; f < frameCount; ++f) {
        float accL = 0.0F;
        float accR = 0.0F;
        for (std::size_t vi = 0; vi < voices.GetSize(); ++vi) {
            AccumulateVoiceForFrame(voices[vi], accL, accR);
        }
        AccumulateVoiceForFrame(music, accL, accR);
        outInterleavedStereo[f * 2U] += accL;
        outInterleavedStereo[f * 2U + 1U] += accR;
    }
}

void SoundMixer::StopAll() noexcept {
    ClearBackgroundMusic();
    for (std::size_t i = 0; i < voices.GetSize(); ++i) {
        voices[i].active = false;
        voices[i].clip = SharedPtr<SoundClip>();
        voices[i].loop = false;
    }
}

}  // namespace Spark
