---
title: Sound Clips
order: 2
---

# Sound Clips

## Class Design: `SoundClip`

Decoded **interleaved stereo float** samples:

```cpp
class SoundClip {
public:
    const Array<float>& GetInterleavedStereo() const noexcept;
    std::uint32_t GetSampleRate() const noexcept;
    std::size_t GetFrameCount() const noexcept;

    static SharedPtr<SoundClip> CreateToneBlip(float frequencyHz,
                                               float durationSeconds, float gain);
    static SharedPtr<SoundClip> CreateSimpleAmbienceLoop();
};
```

Load from file via `GameWorld` or decode helpers in `spark/audio/`.

## Procedural SFX (No Assets)

```cpp
auto jumpSfx = SoundClip::CreateToneBlip(440.0F, 0.08F, 0.5F);
auto landSfx = SoundClip::CreateToneBlip(220.0F, 0.06F, 0.4F);
```

## Load WAV at Runtime

```cpp
// Pattern: load bytes, decode to SoundClip — check SoundClip loaders in spark/audio/
SharedPtr<SoundClip> clip = /* load from assets/audio/jump.wav */;
```

Supported formats depend on build configuration — WAV is always safe.

Next: [Sound Cues](6-sound/03-cues.html).
