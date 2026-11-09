# Sound Clips

`SoundClip` stores **interleaved stereo float PCM**. Load from disk, bundle assets, or procedural generators.

## Concept: load bundled WAV

### C++

```cpp
#include "spark/audio/SoundFileLoader.hpp"

SharedPtr<SoundClip> jump = TryLoadSoundClipFromBundledAsset("assets/audio/jump.wav");
if (!jump)
    jump = SoundClip::CreateToneBlip(520.0F, 0.07F, 0.45F);
```

### C\#

```csharp
var jump = SoundLoader.TryLoadFromBundledAsset("assets/audio/jump.wav")
    ?? SoundClip.CreateToneBlip(520f, 0.07f, 0.45f);
```

### Python

```python
jump = context.load_sound_clip("assets/audio/jump.wav")
```

## Concept: procedural presets

### C++

```cpp
#include "spark/audio/ProceduralSoundPresets.hpp"

auto collect = ProceduralSoundPresets::Get(ProceduralSoundPreset::Collectible);
auto hurt = ProceduralSoundPresets::Get(ProceduralSoundPreset::Hurt);
```

Used in 2D ARPG / platformer when assets absent.

### C\#

```csharp
var collect = ProceduralSoundPresets.Get(ProceduralSoundPreset.Collectible);
```

### Python

```python
# Procedural presets native-only unless bound.
```

## Concept: absolute file path

### C++

```cpp
SharedPtr<SoundClip> clip = TryLoadSoundClipFromFile("/abs/path/custom.wav");
```

### C\#

```csharp
SoundLoader.TryLoadFromFile(path);
```

### Python

```python
pass
```

## Clip lifetime

Store `SharedPtr<SoundClip>` on game state or static cache — queuing copies a pointer, not PCM duplicate.

Next: [Sound Cues](03-cues.md).
