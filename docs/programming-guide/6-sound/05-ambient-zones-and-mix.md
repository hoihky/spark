# Ambient zones and music beds

Interior **muffled** mixes and **looping BGM** stack on top of one-shots. Spark applies **`AmbientZoneComponent`** volumes before cues flush, then mixes **background music** on a separate channel.

**Spatial cues:** [Spatial audio](04-spatial-audio-and-listeners.md).

## Two channels

| Channel | API | Typical content |
|---------|-----|-----------------|
| Bed | `SoundEngine::SetBackgroundMusic` | Level theme loop |
| Events | `SoundCueComponent::Queue*` | Footsteps, impacts, UI |

Zones adjust the **master bed + cue mix** via `AmbientAudioMix` (volume scale + low-pass), not separate clip assets on the component.

---

## Concept: background music lifecycle

Load once in `OnAttach`, clear on scene unload.

### C++

```cpp
void OnAttach(IEngineContext& context) override {
    if (SoundEngine* se = context.TryGetSoundEngine()) {
        if (SharedPtr<SoundClip> bgm = TryLoadSoundClipFromBundledAsset(
                "assets/audio/time_for_adventure.wav")) {
            se->SetBackgroundMusic(bgm, 0.32F, true);
        }
    }
}

void OnDetach(IEngineContext& context) override {
    context.TryGetSoundEngine()->ClearBackgroundMusic();
}
```

`Platformer2DDemo` and P0 path use the same pattern with procedural fallback BGM when assets are absent.

### C\#

```csharp
se.SetBackgroundMusic(bgm, 0.32f, loop: true);
```

### Python

```python
se.set_background_music("assets/audio/time_for_adventure.wav", 0.32, loop=True)
```

---

## Concept: ambient zone (cave / interior)

`ProcessAmbientZones` picks the **highest-priority** zone containing the listener position and writes:

- `volumeScale` — scales perceived loudness (e.g. 0.85 in cave)  
- `lowPassAmount` — muffles highs (underwater, interior)

### C++

```cpp
#include "spark/ecs/components/audio/AmbientZoneComponent.hpp"

GameObject* cave = world.CreateGameObject("CaveReverb");
cave->AddComponent<TransformComponent>()->SetTranslation({8.0F, 0.0F, 0.0F});
auto* zone = cave->AddComponent<AmbientZoneComponent>();
zone->SetHalfExtents({6.0F, 4.0F, 6.0F});
zone->SetVolumeScale(0.85F);
zone->SetLowPassAmount(0.35F);
zone->SetPriority(1);
```

Requires a valid listener pose — mount `AudioListenerComponent` on the camera first.

### C\#

```csharp
cave.AddAmbientZone()
    .SetHalfExtents(new Vector3(6f, 4f, 6f))
    .SetVolumeScale(0.85f)
    .SetLowPassAmount(0.35f);
```

### Python

```python
cave.get_or_add_default_component(ComponentKind.AmbientZone)
```

Overlap two zones: **higher `priority`** wins (same rule as fog/post volumes in [Part 3](../3-3d-graphics/10-post-process-fog-volumes.md)).

---

## Concept: simple music state machine

Spark does not ship a dedicated music FSM component. Recommended pattern:

1. **States:** `Explore`, `Combat`, `Victory` (enum in your `Game` class).  
2. **On enter state:** `SetBackgroundMusic` with crossfade-ready clip swap (stop previous via `ClearBackgroundMusic` or replace clip).  
3. **Combat stinger:** keep bed looping; layer `SoundCueComponent::Queue` for hits.  
4. **Victory:** fade BGM volume in `OnUpdate` while cue plays fanfare one-shot.

### C++

```cpp
void EnterCombat(SoundEngine& se) {
    if (combatBgm) {
        se.SetBackgroundMusic(combatBgm, 0.45F, true);
    }
}
```

### C\#

```csharp
void OnCombatStart() => _sound.SetBackgroundMusic(_combatBgm, 0.45f, loop: true);
```

### Python

```python
def on_victory(self, context):
    context.try_get_sound_engine().clear_background_music()
```

---

## Concept: zone + BGM pairing (level design)

| Zone | `volumeScale` | `lowPassAmount` | BGM note |
|------|---------------|-----------------|----------|
| Outdoor | 1.0 | 0.0 | Full theme |
| Cave mouth | 0.9 | 0.15 | Same clip, zone dip |
| Deep cave | 0.75 | 0.45 | Optional alternate loop |
| Underwater | 0.8 | 0.55 | Swap to underwater bed |

Place zone boxes with the same `Transform` + half-extents workflow as `FogVolumeComponent`.

---

## Demo references

| Demo | Audio pattern |
|------|----------------|
| **#6** Platformer | BGM attach, jump/coin cues, procedural fallback |
| **#7** Broad-phase 2D | Explosion/hurt cues on combat |
| **#8** Maze | Spatial gem collect via `DemoAudio` |
| **#29** Physics | Impact-style feedback (wire contact listener → cue) |

---

## Debugging

| Symptom | Check |
|---------|--------|
| Cave not muffled | Listener inside `halfExtents`? zone enabled? |
| BGM persists | `ClearBackgroundMusic` on scene transition |
| Zones fight | Priorities — raise interior over exterior |
| Cues too quiet | Zone `volumeScale` multiplies mix, not per-cue override |

Part 6 complete → **Part 7**: [Game components usage](../7-game-components-usage/00-introduction.md) · **Part 8**: [2D systems map](../8-2d-game/00-2d-game-systems-map.md).
