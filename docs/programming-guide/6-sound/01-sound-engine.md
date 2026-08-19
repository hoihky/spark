# Sound Engine

## Class Design

```mermaid
flowchart LR
    Cue[SoundCueComponent::Queue] --> Process[ProcessSoundCues]
    Process --> Mixer[SoundMixer]
    Mixer --> Out[Platform Audio Output]
    BGM[SetBackgroundMusic] --> Mixer
    Engine[Engine::PumpFrame] --> Mixer
```

| Class | Role |
|-------|------|
| `SoundEngine` | Facade: startup, pump, BGM |
| `SoundMixer` | Voice pool, mixing |
| `SoundClip` | Decoded stereo float PCM |
| `SoundCueComponent` | Per-entity one-shot queue |

## SoundEngine API

```cpp
class SoundEngine {
public:
    bool Startup();
    void Shutdown() noexcept;
    void PumpFrame(float deltaTimeSeconds) noexcept;
    SoundMixer& GetMixer() noexcept;
    void SetBackgroundMusic(const SharedPtr<SoundClip>& clip,
                            float volume = 0.32F, bool loop = true) noexcept;
    void ClearBackgroundMusic() noexcept;
};
```

`Engine::Run` calls `PumpFrame` after `OnUpdate`. Games access audio via:

```cpp
SoundEngine* audio = context.TryGetSoundEngine();
if (audio) {
    audio->SetBackgroundMusic(ambienceClip, 0.25F, true);
}
```

## Loading and Playing (from `Platformer2DDemo`)

```cpp
#include "spark/audio/SoundFileLoader.hpp"
#include "spark/ecs/components/audio/SoundCueComponent.hpp"

SharedPtr<SoundClip> jumpSfx = TryLoadSoundClipFromBundledAsset("assets/audio/jump.wav");
if (!jumpSfx) {
    jumpSfx = SoundClip::CreateToneBlip(520.0F, 0.07F, 0.45F);  // procedural fallback
}

// One-shot from gameplay:
player->GetComponent<SoundCueComponent>()->Queue(jumpSfx, 0.9F);
// ProcessSoundCues runs automatically in Game::OnUpdate

// Background music in OnAttach:
if (SoundEngine* se = context.TryGetSoundEngine()) {
    se->SetBackgroundMusic(LoadPlatformerBgm(), 0.32F, true);
}
```

Next: [Sound Clips](02-clips.md).
