# Sound Cues

`SoundCueComponent` queues one-shots with gain; `ProcessSoundCues` (in `Game::OnUpdate`) flushes queues through `SoundMixer` with optional 3D pan from `AudioListenerComponent`.

## Concept: play on jump

### C++

```cpp
if (playerInput->WasActionPressedThisFrame("Jump")) {
    playerController->RequestJump();
    if (SoundCueComponent* cue = player->GetComponent<SoundCueComponent>())
        cue->Queue(jumpSfx, 0.9F);
}
```

### C\#

```csharp
if (input.WasActionPressedThisFrame("Jump"))
    player.GetComponent<SoundCueComponent>()?.Queue(jumpSfx, 0.9f);
```

### Python

```python
if inp.was_action_pressed("Jump"):
    player.get_component_sound_cue().queue(jump_clip, 0.9)
```

## Concept: animation events → SFX

Pair `SpriteAnimationEventReceiverComponent` with names like `"Footstep"` and queue cue in `OnSignal` or dedicated VFX/audio component.

### C++

```cpp
if (id == SignalId::SpriteAnimationEvent) {
    const char* name = static_cast<const char*>(payload.ptr);
    if (Utf8String(name) == "Footstep")
        cue->Queue(footstepSfx, 0.6F);
}
```

### C\#

```csharp
if (id == SignalId.SpriteAnimationEvent) { /* queue */ }
```

### Python

```python
def on_signal(self, owner, signal_id, payload):
    pass
```

## Multiple voices

`SoundMixer` multiplexes overlapping one-shots within voice pool limits — rapid `Queue` calls layer instead of cutting (until pool full).

## BGM vs cues

| API | Use |
|-----|-----|
| `SoundEngine::SetBackgroundMusic` | Looping bed |
| `SoundCueComponent::Queue` | Footsteps, jumps, UI blips |

Clear BGM on scene tear-down:

### C++

```cpp
context.TryGetSoundEngine()->ClearBackgroundMusic();
```

### C\#

```csharp
context.TryGetSoundEngine()?.ClearBackgroundMusic();
```

### Python

```python
context.try_get_sound_engine().clear_background_music()
```

Next: [Spatial audio](04-spatial-audio-and-listeners.md) · [Ambient zones & music](05-ambient-zones-and-mix.md).
