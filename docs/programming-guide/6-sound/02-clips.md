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

## Procedural SFX (No Assets)

```cpp
auto jumpSfx = SoundClip::CreateToneBlip(440.0F, 0.08F, 0.5F);
auto landSfx = SoundClip::CreateToneBlip(220.0F, 0.06F, 0.4F);
```

Useful for prototyping — `Platformer2DDemo` falls back to procedural clips when bundled WAV/MP3 files are missing.

## Load WAV/MP3 from Disk

```cpp
#include "spark/audio/SoundFileLoader.hpp"

SharedPtr<SoundClip> clip = TryLoadSoundClipFromFile("/absolute/path/jump.wav");
```

`TryLoadSoundClipFromFile` dispatches by extension (WAV via `TryLoadSoundClipFromWavFile`).

## Load from Bundled Assets

When running inside SparkDemo or a target that sets `SPARK_BUILD_ASSETS_DIR`:

```cpp
SharedPtr<SoundClip> jump = TryLoadSoundClipFromBundledAsset("assets/audio/jump.wav");
SharedPtr<SoundClip> bgm = TryLoadSoundClipFromBundledAsset("assets/audio/time_for_adventure.wav");
```

`Platformer2DDemo` uses this pattern:

```cpp
SharedPtr<SoundClip> LoadPlatformerBgm() {
    SharedPtr<SoundClip> clip =
        TryLoadSoundClipFromBundledAsset("assets/audio/time_for_adventure.wav");
    if (!clip) {
        clip = TryLoadSoundClipFromBundledAsset("assets/audio/time_for_adventure.mp3");
    }
    return clip;
}
```

Queue one-shots at runtime via `SoundCueComponent` — see [Sound Cues](03-cues.md).

Next: [Sound Cues](03-cues.md).
