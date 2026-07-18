---
title: Sound Cues
order: 3
---

# Sound Cues

## Class Design: `SoundCueComponent`

```cpp
class SoundCueComponent final : public GameComponent {
public:
    void Queue(const SharedPtr<SoundClip>& clip, float volume = 1.0F);
    void QueueAtWorld(const SharedPtr<SoundClip>& clip, float volume,
                      const Vector3& worldPosition,
                      float spatialBlend = 1.0F,
                      float minDistance = 1.0F,
                      float maxDistance = 48.0F);
    void FlushTo(SoundEngine* engine) noexcept;
};
```

## Queue Pattern

```cpp
auto* player = playerObject;
auto* cue = player->GetComponent<SoundCueComponent>();
if (!cue) cue = player->AddComponent<SoundCueComponent>();

if (justJumped)
    cue->Queue(jumpClip, 0.9F);
if (justLanded) {
    const Matrix4& wm = player->GetWorldMatrix();
    const Vector3 pos{wm.m[12], wm.m[13], wm.m[14]};
    cue->QueueAtWorld(landClip, 0.6F, pos);
}
```

## Audio Listener

`AudioListenerComponent` marks the owner's transform as the spatial audio listener (highest `priority` wins). Resolved each frame by `ProcessAudioListeners` **before** cues are flushed.

```cpp
cameraGo->AddComponent<AudioListenerComponent>()->SetPriority(10);
```

If no listener is present, spatial panning falls back to the main 3D `CameraComponent` pose.

## Ambient zones

```cpp
cave->AddComponent<AmbientZoneComponent>();
auto* zone = cave->GetComponent<AmbientZoneComponent>();
zone->SetVolumeScale(0.75F);
zone->SetHalfExtents({8.0F, 5.0F, 8.0F});
zone->SetPriority(2);
```

## Automatic Drain

`Game::OnUpdate` calls `ProcessSoundCues(world, context)` which:

1. `ProcessAudioListeners(world)`
2. `ProcessAmbientZones(world)` — regional volume scale when listener is inside `AmbientZoneComponent`
3. `SoundCueComponent::FlushTo` on every active object

If you bypass `Game::OnUpdate`, call `ProcessSoundCues` yourself.

## Background Music

```cpp
if (SoundEngine* se = context.TryGetSoundEngine()) {
    se->SetBackgroundMusic(bgmClip, 0.28F, true);
}
// Later:
se->ClearBackgroundMusic();
```

## Multiple Voices

`SoundMixer` multiplexes one-shots — rapid `Queue` calls overlap without cutting off prior sounds (within voice pool limits). Spatial voices apply stereo pan and distance attenuation from the resolved listener.

Part 6 complete → **Part 7**: [Platformer Introduction](7-2d-game/01-platformer-intro.html).
